extern "C" unsigned avutil_version(void);
extern "C" int clip_test_onnx();

int clip_test_onnx() {
    return avutil_version() == 0;
}
