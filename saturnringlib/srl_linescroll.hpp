#pragma once

#include "srl_base.hpp"
#include "srl_tv.hpp"
#include "srl_debug.hpp"

namespace SRL
{
    
    /** @brief Linescroll Unit Size 
     *  @details A Linescroll table can apply to a ScrollScreen
     *  in chunks of 1, 2, 4, or 8 scanlines. 
     *  If using multi line chunks, requesting or setting data of a line
     *  will read/write the table entry of the line's corresonding chunk.
     */
    enum class LineSize: uint16_t
    {

        Scroll1Line = 0x00,
        Scroll2Line = 0x10,
        Scroll4Line = 0x20,
        Scroll8Line = 0x30,
    };
    /** @brief Linescroll Parameter Types
     *  @details A Linescroll table can store any combination of up to 3 
     *  parameter types per line data entry:
     *  Horizontal Line Offsets (X), Line Substitutions (Y), and Line Scaling Factors(Z).
     *  Use this enum to specify the type of parameter data to be used in the table.
     */
    enum class LineType: uint16_t
    {
        /** @brief Linescroll Data Contains X position offsets 
         *  @details X position offset applies horizontal translation to the 
         *  specified line. 
         *  Pixel at Screen Coordinate(X,Y) becomes data from Coordinate(X + LineData[Y], Y) 
        */
        ScrollX =0x02,
        /** @brief Linescroll Data Contains Y line offsets 
         * @brief Y line substitution replaces image data of one line with image data of another.
         * Pixel at Screen Coordinate(X,Y) gets data from coordinate(X,LineData[Y]).
        */
        ScrollY =0x04,
        /** @brief Linescroll Data Contains Z scaling offsets
         *  @details  Z scaling factors modify the horizontal scaling value of a line by scaling the 
         *  increment by which X values increase along the line. 
         *  Pixel At Screen coordinate(X+dX, Y) becomes data from coordinate(X+(dx*LineData[Y]),Y).
         *  The zoom point originates from the left edge of the screen. 
         *  @note Scaling offsets are subject to the same minimum scale as is set
         *  in the ScrollScreen that the table is registered to, but the Z values here 
         *  represent the inverse of the effective scale
         *  (ie Z value of 2.0 = 1/2x scale, Z value of 0.5 = 2x scale).
         *  Therefore the limits on Z values for each minimum are:
         *  
         *  -1x min:   Z<=1.0
         *  -1/2x min: Z<=2.0
         *  -1/4x min: Z<=4.0
        */
        ScrollZ =0x08,
        /** @brief Linescroll Data contains both X and Y offsets
         * 
        */
        ScrollXY=0x06,
        /** @brief Linescroll Data contains both X and Z offsets 
         * 
        */
        ScrollXZ=0x0a,
        /** @brief Linescroll Data contains both Y and Z offset
         * 
        */
        ScrollYZ=0x0c,
        /** @brief Linescroll Data Contains X, Y , and Z offsets
         * 
        */
        ScrollXYZ=0x0e,
    };
    
    template<LineType T>
    struct LineTraits;

    template<>
    struct LineTraits<LineType::ScrollX> {
        static constexpr int NumParams = 1;
        static constexpr bool hasX = true;
        static constexpr bool hasY = false;
        static constexpr bool hasZ = false;

    };

    template<>
    struct LineTraits<LineType::ScrollY> {
        static constexpr int NumParams = 1;
        static constexpr bool hasX = false;
        static constexpr bool hasY = true;
        static constexpr bool hasZ = false;

    };

    template<>
    struct LineTraits<LineType::ScrollZ> {
        static constexpr int NumParams = 1;
        static constexpr bool hasX = false;
        static constexpr bool hasY = false;
        static constexpr bool hasZ = true;

    };

    template<>
    struct LineTraits<LineType::ScrollXY> {
        static constexpr int NumParams = 2;
        static constexpr bool hasX = true;
        static constexpr bool hasY = true;
        static constexpr bool hasZ = false;

    };

    template<>
    struct LineTraits<LineType::ScrollXZ> {
        static constexpr int NumParams = 2;
        static constexpr bool hasX = true;
        static constexpr bool hasY = false;
        static constexpr bool hasZ = true;

    };

    template<>
    struct LineTraits<LineType::ScrollYZ> {
        static constexpr int NumParams = 2;
        static constexpr bool hasX = false;
        static constexpr bool hasY = true;
        static constexpr bool hasZ = true;
    };

    template<>
    struct LineTraits<LineType::ScrollXYZ> {
        static constexpr int NumParams = 3;
        static constexpr bool hasX = true;
        static constexpr bool hasY = true;
        static constexpr bool hasZ = true;
    };


    constexpr int ChunkSize(LineSize s) {
        switch(s) {
            case LineSize::Scroll1Line: return 1;
            case LineSize::Scroll2Line: return 2;
            case LineSize::Scroll4Line: return 4;
            case LineSize::Scroll8Line: return 8;
        }
        return 1;
    }

    constexpr int ChunkDiv(LineSize s)
    {
        switch(s)
        {
            case LineSize::Scroll1Line: return 0;
            case LineSize::Scroll2Line: return 1;
            case LineSize::Scroll4Line: return 2;
            case LineSize::Scroll8Line: return 3;
        } 
        return 0;
    }

    /* @brief Data Structure to manage a Linescroll table.
    @details Linescroll tables are a VDP2 feature that automates display of raster effects
    without relying on HBlank interrupts. Instead of triggering changes mid-frame with
    interrupts, offsets are preloaded into VDP2 VRAM as a table with one entry 
    per scan line chunk. A ScrollScreen can be then be registered to apply these
    offsets automatically.
    Unlike interrupt-based methods,linescroll tables require explicit values for every line.
    For instance, applying a +10 pixel horizontal offset over 21 scanlines means storing +10
    in each of those 21 table entries, with zeros elsewhere. 
    This class allows for more compact table management by applying user defined 
    functions or lambdas over ranges of the table data for more complex continuous
    raster manipulation.
   
    @note Only NBG0 and NBG1 ScrollScreens can use these line scroll tables. Alternatively,
    manual manipulation of VDP2 registers is still possible with Hblank interrupts and can
    be used to facilitate additional raster effects on all ScrollScreens.
    */    
    template<LineType T, LineSize S>
    class LineScrollTableT
    {
    
        private:
        
        using Traits = LineTraits<T>;
        FIXED* data;
        static constexpr int Chunk = ChunkSize(S);
        static constexpr int Shift = ChunkDiv(S);
        
        public:
        static constexpr LineType type = T;
        static constexpr LineSize size = S;
        int TableSize =  (SRL::TV::GetVerticalRes()>>Shift) * Traits::NumParams;
        LineScrollTableT()
        {
            data = new FIXED[TableSize];
            ResetLines(0, SRL::TV::GetVerticalRes());
        }

        void ResetLines(int16_t start, int16_t end)
        {
            for (int16_t line = start; line < end; line+=ChunkSize(S))
            {
                int base = (line>>Shift) * Traits::NumParams;          
                if constexpr (Traits::hasX) data[base++] = toFIXED(0.0);
                if constexpr (Traits::hasY) data[base++] = (FIXED)(line<<16);
                if constexpr (Traits::hasZ) data[base] = toFIXED(1.0);
            }
        }

        template<typename EffectFn>
        void ApplyEffect(int start, int end, EffectFn&& fn)
        {
            for (int16_t line = start; line < end; line+=ChunkSize(S))
            {
                int base = (line>>Shift) * Traits::NumParams;          
                auto v = fn(line);
                if constexpr (Traits::hasX) data[base++] = v.X.RawValue();
                if constexpr (Traits::hasY) data[base++] = v.Y.RawValue();
                if constexpr (Traits::hasZ) data[base] = v.Z.RawValue();
            }
        }

        /*
        void ApplyOffsetX(int start, int end, Fxp & offset)
        {
            for (int16_t line = start; line < end; line+=ChunkSize(S))
            {   
                int base = (line>>Shift) * Traits::NumParams;          
                if constexpr (Traits::hasX) data[base++] = offset.RawValue();
            }
        }*/

        FIXED* GetData()
        {
            return this->data;
        }
    };


    
    
    
   

    struct LineScrollTable
    {
        private:
        FIXED* Data;
        LineSize Size;
        LineType Type;
        int8_t Xdata;
        int8_t Ydata;
        int8_t Zdata;
        int8_t LineDataWidth;
        int8_t LineChunk;
        
        public:
        int TableSize;
        //initialize empty linescroll table with no data 
        LineScrollTable()
        {
            Size = LineSize::Scroll1Line;
            Type = LineType::ScrollX;
            Xdata = 0;
            Ydata = Zdata = -1; 
            Data = nullptr;
        }
        /** @brief initialize linescroll table of specified line size and type:
        *  
        *   @details Allocates table of Linescroll data and initializes to 
        *   default data values based on display resolution and the type and size specified.
        *   -mem footprint = (4*(#params_per_entry)*(vertical_screen_res))/(#lines_per_chunk) Bytes
        *   -X offset data is initialized to Fxp(0.0) if present
        *   -Y line data is initialized to Fxp(table_index) if present
        *   -Z zoom data is initailized to Fxp(1.0) if present
        */
        LineScrollTable(LineType type, LineSize size)
        {
            Type = type;
            Size = size;
            int8_t i=0;
            Xdata=Ydata =Zdata = -1;
            if((uint16_t)Type&0x2) Xdata = i++;
            if((uint16_t)Type&0x4) Ydata = i++;
            if((uint16_t)Type&0x8) Zdata = i++;
            LineDataWidth = i;
            uint16_t numLines = SRL::TV::GetVerticalRes();

            switch(Size) 
            {
                case(LineSize::Scroll1Line):
                    LineChunk = 1;
                    TableSize = (LineDataWidth*numLines);
                    Data = autonew FIXED[LineDataWidth*numLines];
                    break;
                case(LineSize::Scroll2Line):
                    LineChunk = 2;
                    TableSize = (LineDataWidth*numLines)>>1;
                    Data = autonew FIXED[(LineDataWidth*numLines)>>1];
                    break;
                case(LineSize::Scroll4Line):
                    LineChunk = 4;
                    TableSize = (LineDataWidth*numLines)>>2;
                    Data = autonew FIXED[(LineDataWidth*numLines)>>2];
                    break;
                case(LineSize::Scroll8Line):
                    LineChunk = 8;
                    TableSize = (LineDataWidth*numLines)>>3;
                    Data = autonew FIXED[(LineDataWidth*numLines)>>3];
                    break;
            }
            ResetLines(0,numLines);
        }

        ~LineScrollTable()
        {
            delete(Data);
        }
        
        /** @brief Resets all LineData from Line Start to Line End with default values
        *   
        *   @details
        *   Each data parameter present in line data within the specified range is reset
        *   to the value that leaves the final display unchanged (as though no line scroll was applied)
        *   X offset data  = Fxp(0.0) 
        *   Y line number = Fxp(table_index) 
        *   Z Scaling data  = Fxp(1.0) 
        */
        void ResetLines(uint16_t Start,uint16_t End)
        {
            switch(Size) 
            {
                case(LineSize::Scroll1Line): 
                    break;
                case(LineSize::Scroll2Line):
                    Start>>=1;
                    End>>=1;
                    break;
                case(LineSize::Scroll4Line):
                    Start>>=2;
                    End>>=2;
                    break;
                case(LineSize::Scroll8Line):
                    Start>>=3;
                    End>>=3;
                    break;
            }
            for(int i = Start; i<End; ++i)
            {
                //this->Data[i] //toFIXED(0.0); 
                if((uint16_t)Type & 0x02) this->Data[(i*LineDataWidth)] = 0;
                if((uint16_t)Type & 0x04) this->Data[(i*LineDataWidth)+Ydata] = (FIXED)(i<<16);
                if((uint16_t)Type & 0x08) this->Data[(i*LineDataWidth)+Zdata] = toFIXED(1.0);
            }
        }
        
        /** @brief Allows user to apply custom line effect functions to the line scroll table
        *   @details User effect function must accept a single uint16_t argument representing
        *   the input line and return a Vector3d object containing the desired line table offsets
        *   for the input line. If the LineScroll Data is not configured to contain a data entry
        *   for one of the returned Vector's components, that component will be ingnored. 
        *   within the range of Startline to Endline, The effect function will be evaluated
        *   for each line that is a Multiples of linesize.
        */
        
        void ApplyLineEffect(uint16_t StartLine, uint16_t EndLine, SRL::Math::Types::Vector3D (*effect) (uint16_t line))
        {
            SRL::Math::Types::Vector3D lineOutput;
            uint16_t lineInc;
           
            switch(Size) 
            {
                case(LineSize::Scroll1Line):
                    lineInc = 1;
                    break;
                case(LineSize::Scroll2Line):
                    lineInc = 2;
                    StartLine &=0xfffe;
                    EndLine &= 0xfffe;
                    break;
                case(LineSize::Scroll4Line):
                    lineInc = 4;
                    StartLine &=0xfffd;
                    EndLine &= 0xfffd;
                    break;
                case(LineSize::Scroll8Line):
                    lineInc = 8;
                    StartLine &=0xfffc;
                    EndLine &= 0xfffc;
                    break;
            }
            for(int i = StartLine; i<EndLine; i+=lineInc)
            {
                lineOutput = effect(i);
                if((uint16_t)Type & 0x02) this->Data[(i*LineDataWidth)] = lineOutput.X.RawValue();
                if((uint16_t)Type & 0x04) this->Data[(i*LineDataWidth)+Ydata] = lineOutput.Y.RawValue();
                if((uint16_t)Type & 0x08) this->Data[(i*LineDataWidth)+Zdata] = lineOutput.Z.RawValue(); 
            }
        }
       
        
        FIXED* GetData()
        {
            return this->Data;
        }

        LineSize GetLineSize()
        {
            return this->Size;
        }
        LineType GetLineType()
        {
            return this->Type;
        }
    
    };
}