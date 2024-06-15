#if		!defined( __MACHINE_H )
	#define	__MACHINE_H
	
	static inline void	nop( void ){
		__asm__ volatile ( "nop": : );
	}
	static inline void	sleep( void ){
		__asm__ volatile ( "sleep": : );
	}
	static inline int	tas( unsigned char	*b ){
		__asm__ volatile ( "tas.b\t@%0": : "r" (b) );
		return	( int )( *b );
	}
	static inline void	set_sr( unsigned int	sr ){
		__asm__ volatile ( "ldc\t%0,sr": : "r" (sr) );
	}
	static inline unsigned int	get_sr( void ){
		unsigned int	sr;
		
		__asm__ volatile ( "stc\tsr,%0": "=r" (sr) );
		return	sr;
	}
	static inline unsigned int	get_imask( void ){
		unsigned int	imask = ( get_sr() & 0x000000f0 ) >> 4;
		
		return	imask;
	}
	static inline void	set_imask( unsigned int	imask ){
		unsigned int	sr = get_sr();
		
		sr &= ~0x000000f0;
		sr |= ( imask << 4 );
		set_sr( sr );
	}
	static inline void	set_vbr( void	*vbr ){
		__asm__ volatile ( "ldc\t%0,vbr": : "r" (vbr) );
	}
	static inline void	*get_vbr( void ){
		void	*vbr;
		
		__asm__ volatile( "stc\tvbr,%0": "=r" (vbr) );
		return	vbr;
	}
	static inline void	set_gbr( void	*gbr ){
		__asm__ volatile ( "ldc\t%0,gbr": : "r" (gbr) );
	}
	static inline void	*get_gbr( void ){
		void	*gbr;
		
		__asm__ volatile( "stc\tgbr,%0": "=r" (gbr) );
		return	gbr;
	}
	
#endif
