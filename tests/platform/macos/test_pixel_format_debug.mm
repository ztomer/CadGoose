#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#include <cstdio>
#include <vector>
#include <cstring>

@interface CaptureDelegate : NSObject <SCStreamOutput>
@property dispatch_semaphore_t done;
@property (atomic) std::vector<uint8_t>* captured;
@property (atomic) int* capW;
@property (atomic) int* capH;
@property (atomic) uint32_t* fmtOut;
@end

@implementation CaptureDelegate
- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sb
         ofType:(SCStreamOutputType)type {
    if (type != SCStreamOutputTypeScreen) return;
    CVPixelBufferRef pb = CMSampleBufferGetImageBuffer(sb);
    if (!pb) return;

    OSType fmt = CVPixelBufferGetPixelFormatType(pb);
    int w = (int)CVPixelBufferGetWidth(pb);
    int h = (int)CVPixelBufferGetHeight(pb);
    size_t bpr = CVPixelBufferGetBytesPerRow(pb);

    printf("\n=== Captured Frame ===\n");
    printf("Format: 0x%x (", (unsigned)fmt);
    char fcc[5] = { (char)(fmt>>24), (char)(fmt>>16), (char)(fmt>>8), (char)(fmt), 0 };
    printf("%s", fcc);
    printf(")\n");
    printf("Dimensions: %dx%d\n", w, h);
    printf("BytesPerRow: %zu (computed stride: %d)\n", bpr, w*4);

    *self.fmtOut = fmt;
    *self.capW = w;
    *self.capH = h;
    self.captured->resize(h * w * 4);

    CVPixelBufferLockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);
    uint8_t* src = (uint8_t*)CVPixelBufferGetBaseAddress(pb);
    for (int y = 0; y < h; ++y) {
        memcpy(&(*self.captured)[y * w * 4], src + y * bpr, w * 4);
    }
    CVPixelBufferUnlockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);

    // Print first 4 pixels from captured buffer
    printf("First 4 pixels (from captured vector, byte0..3):\n");
    for (int i = 0; i < 4 && i < w; i++) {
        size_t idx = i * 4;
        printf("  Pixel %d: [0]=%d [1]=%d [2]=%d [3]=%d\n",
               i, (*self.captured)[idx], (*self.captured)[idx+1],
               (*self.captured)[idx+2], (*self.captured)[idx+3]);
    }

    dispatch_semaphore_signal(self.done);
}
@end

static int CountCyan(const uint8_t* p, int w, int h) {
    int cnt = 0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            size_t i = (size_t)(y * w + x) * 4;
            if (p[i+1] > 200 && p[i] > 200 && p[i+2] < 50) cnt++;
        }
    return cnt;
}

int main() {
    [NSApplication sharedApplication];

    __block SCShareableContent* content = nil;
    dispatch_semaphore_t sema = dispatch_semaphore_create(0);
    [SCShareableContent getShareableContentWithCompletionHandler:
        ^(SCShareableContent* _Nullable sc, NSError* _Nullable err) {
            content = sc;
            dispatch_semaphore_signal(sema);
        }];
    dispatch_semaphore_wait(sema, dispatch_time(DISPATCH_TIME_NOW, 5*NSEC_PER_SEC));
    if (!content || content.displays.count == 0) {
        printf("FAIL: No display content\n");
        return 1;
    }

    SCDisplay* display = content.displays.firstObject;
    printf("Display: %ld x %ld (points)\n", (long)display.width, (long)display.height);

    SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
    config.width = display.width;
    config.height = display.height;
    config.pixelFormat = kCVPixelFormatType_32BGRA;
    config.showsCursor = NO;
    config.capturesAudio = NO;
    config.minimumFrameInterval = CMTimeMake(1, 30);

    SCContentFilter* filter = [[SCContentFilter alloc]
        initWithDisplay:display excludingWindows:@[]];

    std::vector<uint8_t> captured;
    int capW = 0, capH = 0;
    uint32_t actualFormat = 0;

    CaptureDelegate* del = [[CaptureDelegate alloc] init];
    del.done = dispatch_semaphore_create(0);
    del.captured = &captured;
    del.capW = &capW;
    del.capH = &capH;
    del.fmtOut = &actualFormat;

    SCStream* stream = [[SCStream alloc] initWithFilter:filter
                                          configuration:config
                                               delegate:nil];
    dispatch_queue_t q = dispatch_queue_create("capture", DISPATCH_QUEUE_SERIAL);
    [stream addStreamOutput:del type:SCStreamOutputTypeScreen
          sampleHandlerQueue:q error:nil];
    [stream startCaptureWithCompletionHandler:^(NSError* err) {
        if (err) printf("Start error: %s\n", err.localizedDescription.UTF8String);
    }];

    dispatch_semaphore_wait(del.done,
        dispatch_time(DISPATCH_TIME_NOW, 3*NSEC_PER_SEC));
    [stream stopCaptureWithCompletionHandler:nil];

    if (!captured.empty() && capW > 0) {
        int cyan = CountCyan(captured.data(), capW, capH);
        printf("\nCountCyan on captured vector: %d\n", cyan);

        // Save as PNG using BGRA setup
        CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
        CGContextRef ctx = CGBitmapContextCreate(
            (void*)captured.data(), capW, capH, 8, capW * 4, cs,
            (CGBitmapInfo)kCGImageAlphaPremultipliedFirst |
                kCGBitmapByteOrder32Little);
        CGColorSpaceRelease(cs);
        if (ctx) {
            // Check context data
            const uint8_t* ctxData = (const uint8_t*)CGBitmapContextGetData(ctx);
            int cyanFromCtx = CountCyan(ctxData, capW, capH);
            printf("CountCyan from context data: %d\n", cyanFromCtx);

            printf("Context first 4 pixels:\n");
            for (int i = 0; i < 4 && i < capW; i++) {
                size_t idx = i * 4;
                printf("  Pixel %d: [0]=%d [1]=%d [2]=%d [3]=%d\n",
                       i, ctxData[idx], ctxData[idx+1],
                       ctxData[idx+2], ctxData[idx+3]);
            }

            CGImageRef img = CGBitmapContextCreateImage(ctx);
            if (img) {
                printf("CGImage bitmapInfo: 0x%x\n", (unsigned)CGImageGetBitmapInfo(img));
                NSURL* url = [NSURL fileURLWithPath:@"/tmp/test_captured_frame.png"];
                CGImageDestinationRef dest = CGImageDestinationCreateWithURL(
                    (__bridge CFURLRef)url, (__bridge CFStringRef)@"public.png", 1, NULL);
                if (dest) {
                    CGImageDestinationAddImage(dest, img, NULL);
                    CGImageDestinationFinalize(dest);
                    CFRelease(dest);
                }
                CGImageRelease(img);
            }
            CGContextRelease(ctx);
        }
        printf("PNG saved to /tmp/test_captured_frame.png\n");
    }
    return 0;
}
