//Download by http://www.NewXing.com
/***************************************************************/
 
/***************************************************************/
 
 
 
#ifndef _CONVERT_H
#define _CONVERT_H
 
 
class ColorSpaceConversions {
 
public:
 
    ColorSpaceConversions();
 
    void RGB24_to_YV12(unsigned char * in, unsigned char * out,int w, int h);
    void YV12_to_RGB24(unsigned char *src0,unsigned char *src1,unsigned char *src2,unsigned char *dst_ori,int width,int height);
    void YVU9_to_YV12(unsigned char * in,unsigned char * out, int w, int h);
    void YUY2_to_YV12(unsigned char * in,unsigned char * out, int w, int h);
    void YV12_to_YVU9(unsigned char * in,unsigned char * out, int w, int h);
    void YV12_to_YUY2(unsigned char * in,unsigned char * out, int w, int h);
};
 
#endif /* _CONVERT_H */


