import objc
from AppKit import NSImage, NSView, NSRect, NSGraphicsContext, NSColor, NSFont, NSAttributedString, NSMutableDictionary
import CoreGraphics as CG
import Quartz

def test_draw():
    # Create an image to draw into
    img = NSImage.alloc().initWithSize_((200, 200))
    img.lockFocus()
    ctx = NSGraphicsContext.currentContext().CGContext()
    
    # Simulate drawRect from renderer.mm
    CG.CGContextTranslateCTM(ctx, 0, 200)
    CG.CGContextScaleCTM(ctx, 1.0, -1.0)
    
    # Simulate debugoose text
    font = Quartz.CTFontCreateWithName(b"Courier", 20.0, None)
    white = Quartz.CGColorCreateGenericRGB(1.0, 0.0, 0.0, 1.0) # Red to see it
    
    keys = [Quartz.kCTFontAttributeName, Quartz.kCTForegroundColorAttributeName]
    values = [font, white]
    attrs = dict(zip(keys, values))
    
    string = Quartz.CFStringCreateWithCString(None, b"Hello", Quartz.kCFStringEncodingUTF8)
    attrStr = Quartz.CFAttributedStringCreate(None, string, attrs)
    line = Quartz.CTLineCreateWithAttributedString(attrStr)
    
    CG.CGContextSetTextMatrix(ctx, CG.CGAffineTransformMakeScale(1.0, -1.0))
    CG.CGContextSetTextPosition(ctx, 20, 50)
    Quartz.CTLineDraw(line, ctx)
    
    # Simulate DrawDroppedItem text
    text = "World"
    textAttrs = {
        "NSFont": NSFont.systemFontOfSize_(20.0),
        "NSColor": NSColor.blueColor()
    }
    
    CG.CGContextSaveGState(ctx)
    CG.CGContextTranslateCTM(ctx, 20, 100 + 30)
    CG.CGContextScaleCTM(ctx, 1.0, -1.0)
    
    # Draw text using AppKit
    attrStr2 = NSAttributedString.alloc().initWithString_attributes_(text, textAttrs)
    attrStr2.drawInRect_(((0, 0), (100, 30)))
    
    CG.CGContextRestoreGState(ctx)
    
    img.unlockFocus()
    
    # Save to file
    data = img.TIFFRepresentation()
    rep = AppKit.NSBitmapImageRep.imageRepWithData_(data)
    pngData = rep.representationUsingType_properties_(AppKit.NSBitmapImageFileTypePNG, None)
    pngData.writeToFile_atomically_("test.png", True)

test_draw()
print("Saved to test.png")
