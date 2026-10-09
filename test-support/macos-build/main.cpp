#include <QApplication>

extern "C" int clip_test_onnx();

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    return clip_test_onnx();
}
