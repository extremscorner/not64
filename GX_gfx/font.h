/*****************************************************************************
 * font.h
 *****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
extern void init_font(void);
void write_font(int x, int y, char *string,u32 **axfb,int whichfb);

#ifdef __cplusplus
}
#endif
