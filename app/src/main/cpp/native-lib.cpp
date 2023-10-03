#include <jni.h>
#include <android/bitmap.h>
#include <string>

namespace asdkwrapper {


    class Helper {

    protected:
        Helper(JNIEnv *env, jclass clazz) : mEnv(env), mClazz(clazz) {

        }

        Helper(JNIEnv *env, const char *className) : mEnv(env) {
            mClazz = findClass(className);
        }

        Helper(Helper &helper, jclass clazz) : mEnv(helper.mEnv) {
            mClazz = clazz;
        }

        Helper(Helper &helper, const char *className) : mEnv(helper.mEnv) {
            mClazz = findClass(className);
        }

        jclass findClass(const char *className) {
            jclass clazz = mEnv->FindClass(className);
            if (clazz == nullptr) {
                throw std::runtime_error("Cannot find class");
            }
            return clazz;
        }

        jmethodID getMethodID(jclass clazz, const char *methodName, const char *methodSignature) {
            jmethodID methodID = mEnv->GetMethodID(clazz, methodName, methodSignature);
            if (methodID == nullptr) {
                throw std::runtime_error("Cannot find method");
            }
            return methodID;
        }

        jmethodID getMethodID(const char *methodName, const char *methodSignature) {
            return getMethodID(mClazz, methodName, methodSignature);
        }

    public:
        JNIEnv *getEnv() {
            return mEnv;
        }

        jclass getClazz() {
            return mClazz;
        }

    private:
        JNIEnv *mEnv;
        jclass mClazz;
    };

    class ParcelFileDescriptor : public Helper {

    public:
        ParcelFileDescriptor(Helper &helper, jobject parcelFileDescriptor) : Helper(helper, "android/os/ParcelFileDescriptor") {
            mParcelFileDescriptor = parcelFileDescriptor;
            mMID_getFd = getMethodID("getFd", "()I");
        }

        int getFd() {
            return getEnv()->CallIntMethod(mParcelFileDescriptor, mMID_getFd);
        }

    private:
        jobject mParcelFileDescriptor;
        jmethodID mMID_getFd;

    };

    class ContentResolver : public Helper {

    public:
        ContentResolver(Helper &helper, jobject contentResolver) : Helper(helper, "android/content/ContentResolver") {
            mContentResolver = contentResolver;
            mMID_openFileDescriptor = getMethodID("openFileDescriptor", "(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;");
        }

        ParcelFileDescriptor openFileDescriptor(jobject uri, const char *mode) {
            jobject parcelFileDescriptor = getEnv()->CallObjectMethod(mContentResolver, mMID_openFileDescriptor, uri, getEnv()->NewStringUTF(mode));
            return ParcelFileDescriptor(*this, parcelFileDescriptor);
        }

        ParcelFileDescriptor openFileDescriptor(const char *uri, const char *mode) {
            jobject parcelFileDescriptor = getEnv()->CallObjectMethod(mContentResolver, mMID_openFileDescriptor, getEnv()->NewStringUTF(uri), getEnv()->NewStringUTF(mode));
            return ParcelFileDescriptor(*this, parcelFileDescriptor);
        }

    private:
        jobject mContentResolver;
        jmethodID mMID_openFileDescriptor;
    };

    class Context : public Helper {

    public:
        Context(JNIEnv *env) : Helper(env, "android/content/Context") {
            mMID_getContentResolver = getMethodID("getContentResolver", "()Landroid/content/ContentResolver;");
        }

        ContentResolver getContentResolver() {
            jobject contentResolver = getEnv()->CallObjectMethod(getClazz(), mMID_getContentResolver);
            return ContentResolver(*this, contentResolver);
        }

    private:
        jmethodID mMID_getContentResolver;

    };

    class Bitmap : public Helper {

    public:
        Bitmap(Helper &helper, jobject bitmap) : Helper(helper, "android/graphics/Bitmap") {
            mBitmap = bitmap;
            initializeMethods();
        }

        Bitmap(Helper &helper, int width, int height) : Helper(helper, "android/graphics/Bitmap") {
            // by default, use ARGB_8888
            jobject config = getEnv()->GetStaticObjectField(getClazz(), getEnv()->GetStaticFieldID(getClazz(), "Config", "Landroid/graphics/Bitmap$Config;"));
            mBitmap = getEnv()->CallStaticObjectMethod(getClazz(), getMethodID("createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;"), width, height, config);
            initializeMethods();
        }

        void initializeMethods() {
            mMID_getWidth = getMethodID("getWidth", "()I");
            mMID_getHeight = getMethodID("getHeight", "()I");
            mMID_getConfig = getMethodID("getConfig", "()Landroid/graphics/Bitmap$Config;");
            mMID_getPixels = getMethodID("getPixels", "([IIIIIII)V");
        }

        int getWidth() {
            return getEnv()->CallIntMethod(mBitmap, mMID_getWidth);
        }

        int getHeight() {
            return getEnv()->CallIntMethod(mBitmap, mMID_getHeight);
        }

        jobject getConfig() {
            return getEnv()->CallObjectMethod(mBitmap, mMID_getConfig);
        }

        char *toNewArray() {
            int width = getWidth();
            int height = getHeight();
            int stride = width * 4;
            char *pixels = new char[stride * height];
            getEnv()->CallVoidMethod(mBitmap, mMID_getPixels, pixels, 0, stride, 0, 0, width, height);
            return pixels;
        }

    private:
        jobject mBitmap;
        jmethodID mMID_getWidth;
        jmethodID mMID_getHeight;
        jmethodID mMID_getConfig;
        jmethodID mMID_getPixels;

    };

    class PdfRenderer : public Helper {

    public:
        class Page : public Helper {

            friend class PdfRenderer;

        protected:
            Page(Helper &helper, jobject page) : Helper(helper, "android/graphics/pdf/PdfRenderer$Page") {
                mPage = page;
                mMID_render = getMethodID("render", "(Landroid/graphics/Bitmap;Landroid/graphics/Rect;Landroid/graphics/pdf/PdfRenderer$Page$RenderMode;)V");
            }

        public:
            int getWidth(int dpi = 72) {
                return getEnv()->CallIntMethod(mPage, mMID_getWidth, dpi);
            }

            int getHeight(int dpi = 72) {
                return getEnv()->CallIntMethod(mPage, mMID_getHeight, dpi);
            }

            Bitmap render(int dpi = 72) {
                int width = getWidth(dpi);
                int height = getHeight(dpi);
                Bitmap bitmap(*this, width, height);
                getEnv()->CallVoidMethod(mPage, mMID_render, bitmap.getClazz(), nullptr, nullptr);
                return bitmap;
            }

        private:
            jobject mPage;
            jmethodID mMID_render;
            jmethodID mMID_getWidth;
            jmethodID mMID_getHeight;

        };

    public:
        PdfRenderer(Helper &helper, ParcelFileDescriptor &file) : Helper(helper, "android/graphics/pdf/PdfRenderer") {
            mPdfRenderer = getEnv()->NewObject(getClazz(), getMethodID("<init>", "(Landroid/os/ParcelFileDescriptor;)V"), file.getFd());
            mMID_getPageCount = getMethodID("getPageCount", "()I");
            mMID_openPage = getMethodID("openPage", "(I)Landroid/graphics/pdf/PdfRenderer$Page;");
        }

        int pageCount() {
            return getEnv()->CallIntMethod(mPdfRenderer, mMID_getPageCount);
        }

        Page openPage(int index) {
            jobject page = getEnv()->CallObjectMethod(mPdfRenderer, mMID_openPage, index);
            return Page(*this, page);
        }

    private:
        jobject mPdfRenderer;
        jmethodID mMID_getPageCount;
        jmethodID mMID_openPage;

    };

}

extern "C" JNIEXPORT jstring JNICALL
Java_fr_lizabelos_asdkcppwrapper_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {

    // test render a pdf named test.pdf from the assets folder into a pixel array using the wrappers
    asdkwrapper::Context context(env);
    asdkwrapper::ContentResolver contentResolver = context.getContentResolver();
    asdkwrapper::ParcelFileDescriptor parcelFileDescriptor = contentResolver.openFileDescriptor("file:///android_asset/test.pdf", "r");

    asdkwrapper::PdfRenderer pdfRenderer(context, parcelFileDescriptor);
    asdkwrapper::PdfRenderer::Page page = pdfRenderer.openPage(0);
    asdkwrapper::Bitmap bitmap = page.render(); // or page.render(300) to render at 300 dpi
    char *pixels = bitmap.toNewArray();



    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}