#include <stdio.h>
#include "libyuv.h"
#include <string.h>

using namespace libyuv;

int static saveFile(uint8_t * data , uint32_t size , char* filename){
    int ret = -1;
    char file_path[64];
    sprintf(file_path, "%s/%s_%s.yuv", "/data/local/tmp/images", "image_yuv", filename);

    FILE *out_file = fopen(file_path, "wb");
    if (!out_file) {
        printf("failed to create out_file.\n");
        return -1;
    }

    ret = fwrite(data, 1, size, out_file);
    if (ret < 0) {
        printf("failed to write argb to file. %d\n", ret);
        return -1;
    }

    fclose(out_file);

    return ret;

}

/**
 * @brief
 * NV21 mirror -> rotate -> scale
 * @param in_nv21
 * @param out_nv21
 * @param w
 * @param h
 */
int NV21Convert(uint8_t *in_nv21, uint8_t *out_nv21, uint32_t w, uint32_t h) {
    int ret = -1;
    uint32_t ysize, usize, vsize;
    uint32_t ystride, ustride, vstride;
    uint32_t i420size;

    uint8_t *i420data = nullptr;
    uint8_t *i420dataMirror = nullptr;
    uint8_t *i420dataOriate = nullptr;
    uint8_t *i420dataScale = nullptr;

    ystride = w;
    ustride = w >> 1;
    vstride = w >> 1;

    ysize = h * ystride;
    usize = (h >> 1) * ustride;
    vsize = (h >> 1) * vstride;

    i420size = ysize + usize + vsize;

    i420data = (uint8_t *)malloc(i420size);
    ret = NV21ToI420( in_nv21, ystride, 
                in_nv21 + ysize, ustride + vstride,
                i420data, ystride,
                i420data + ysize, ustride, 
                i420data + ysize + usize, vstride, 
                w, h);
    saveFile(i420data, i420size, "i420");
    if (ret != 0){
        goto exit;
    }
    
    i420dataMirror = (uint8_t *)malloc(i420size);
    printf("start I420Mirror \n");
    ret = I420Mirror(i420data, ystride,
               i420data + ysize , ustride,
               i420data + ysize + usize, vstride,
               i420dataMirror, ystride,
               i420dataMirror + ysize, ustride,
               i420dataMirror + ysize + usize , vstride,
               w, h);
    printf("end I420Mirror ret: %d \n" , ret);
    saveFile(i420dataMirror, i420size, "mirror");
    if (ret != 0){
        goto exit;
    }

    i420dataOriate = (uint8_t *)malloc(i420size);
    printf("start I420Rotate \n");
    ret = I420Rotate(i420dataMirror, ystride,
               i420dataMirror + ysize, ustride,
               i420dataMirror + ysize + usize , vstride,
               i420dataOriate, h,
               i420dataOriate + ysize, h >> 1,
               i420dataOriate + ysize + usize , h >>1,
               w, h,
               kRotate90 );
    printf("end I420Rotate ret: %d \n" , ret);
    saveFile(i420dataOriate, i420size, "rotate");
    if (ret != 0){
        goto exit;
    }

    i420dataScale = (uint8_t *)malloc(i420size);
    printf("start I420Scale \n");
    
    ret = I420Scale(i420dataOriate, h,
              i420dataOriate + ysize,  h >> 1,
              i420dataOriate + ysize + usize ,  h >> 1,
              h, w,
              i420dataScale , ystride,
              i420dataScale + ysize , ustride,
              i420dataScale + ysize + usize , vstride,
              w, h,
              FilterMode::kFilterNone);
    printf("end I420Scale ret: %d \n" , ret);
    saveFile(i420dataScale, i420size, "scale");
    if (ret != 0){
        goto exit;
    }


    printf("start I420ToNV21 \n");
    ret = I420ToNV21(i420dataScale, ystride,
                i420dataScale + ysize, ustride,
                i420dataScale + ysize + usize, vstride,
                out_nv21, ystride,
                out_nv21 + ysize , ustride + vstride,
                w, h);
    printf("end I420ToNV21 ret: %d \n" , ret);
   
exit:

    if (i420data != nullptr) {
        free(i420data);
        i420data = nullptr;
    }
    
    if (i420dataMirror != nullptr) {
        free(i420dataMirror);
        i420dataMirror = nullptr;
    }
        
    if (i420dataOriate != nullptr) {
        free(i420dataOriate);
        i420dataOriate = nullptr;
    }
        
    if (i420dataScale != nullptr) {
        free(i420dataScale);
        i420dataScale = nullptr;
    }

    return ret;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("example\n   %s 640x480 yuvfile argbfile\n", argv[0]);
        return -1;
    }

    int ret;

    int width, height;
    sscanf(argv[1], "%dx%d", &width, &height);

    char *yuv_file_path = argv[2];
    char *argb_file_path = argv[3];

    FILE *fp = fopen(yuv_file_path, "rb");
    if (!fp)
    {
        printf("failed to read file in path: %s\n", yuv_file_path);
        return -1;
    }

    int y_buf_size = width * height;
    uint8_t y_buf[y_buf_size];

    ret = fread(y_buf, 1, y_buf_size, fp);
    if (ret != y_buf_size)
    {
        printf("failed to read y data.\n");
        return -1;
    }

    int vu_buf_size = width * height / 2;
    uint8_t vu_buf[vu_buf_size];

    ret = fread(vu_buf, 1, vu_buf_size, fp);
    if (ret != vu_buf_size)
    {
        printf("failed to read vu data.\n");
        return -1;
    }

    fclose(fp);

    uint32_t nv21_size = y_buf_size + vu_buf_size;
    uint8_t  nv21_in[y_buf_size + vu_buf_size];
    uint8_t nv21_out[y_buf_size + vu_buf_size];

    memcpy(nv21_in, y_buf, y_buf_size);
    memcpy(nv21_in + y_buf_size, vu_buf, vu_buf_size);

    saveFile(nv21_in, nv21_size, "nv21origin-in");

    NV21Convert(nv21_in, nv21_out,width, height);

    saveFile(nv21_out, nv21_size, "nv21ortate-out");
    
    int argb_size = width * height * 4;
    uint8_t argb_buf[argb_size];

    // ret = NV21ToARGB(y_buf, width, vu_buf, width, argb_buf, width * 4, width, height);
    ret = NV21ToABGR(nv21_out, height, nv21_out + y_buf_size, height, argb_buf, height * 4,
                     height, width);
    if (ret != 0)
    {
        printf("failed to NV21ToARGB. %d\n", ret);
        return -1;
    }

    FILE *out_file = fopen(argb_file_path, "wb");
    if (!out_file)
    {
        printf("failed to create argb file.\n");
        return -1;
    }

    ret = fwrite(argb_buf, 1, argb_size, out_file);
    if (ret < 0)
    {
        printf("failed to write argb to file. %d\n", ret);
        return -1;
    }

    fclose(out_file);

    printf("convert success.\n");

    return 0;
}
