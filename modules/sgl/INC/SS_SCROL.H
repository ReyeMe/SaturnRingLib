#define         YOKO                      0
#define         TATE                      1
#define         SIPOSX                    toFIXED(-35.0)
#define         SIPOSY                    toFIXED(-50.0)
#define         SX                        toFIXED(512.0)
#define         SY                        toFIXED(512.0)
#define         POSX_UP                   toFIXED(2.0)
#define         POSY_UP                   toFIXED(2.0) 

extern          uint8_t    yama_cel[] ;
extern          uint16_t   yama_map[] ;
extern          uint16_t   yama_pal[] ;
extern          uint8_t    kage_cel[] ;
extern          uint16_t   kage_map[] ;
extern          uint16_t   kage_pal[] ;
extern          uint8_t    am2_cel[] ;
extern          uint16_t   am2_map[] ;
extern          uint16_t   am2_pal[] ;
extern          uint16_t   ascii_map[] ;
extern          uint8_t    sora_cel[] ;
extern          uint16_t   sora_map[] ;
extern          uint16_t   sora_pal[] ;
extern          uint8_t    sora_cel2[] ;
extern          uint16_t   sora_map2[] ;
extern          uint16_t   sora_pal2[] ;
extern          uint8_t    sima_cel[] ;
extern          uint16_t   sima_map[] ;
extern          uint16_t   sima_pal[] ;
extern          uint8_t    tuti_cel[] ;
extern          uint16_t   tuti_map[] ;
extern          uint16_t   tuti_pal[] ;
extern          uint8_t    tuti_cel2[] ;
extern          uint16_t   tuti_map2[] ;
extern          uint16_t   tuti_pal2[] ;

extern          void Cel2VRAM(uint8_t *  , void * , uint32_t ) ;
extern          void Map2VRAM(uint16_t * , void * , uint16_t , uint16_t , uint16_t , uint32_t ) ;
extern          void Pal2CRAM(uint16_t * , void * , uint32_t ) ;
