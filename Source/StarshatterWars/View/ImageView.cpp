#include "ImageView.h"
#include "Video.h"
#include "Bitmap.h"

// --------------------------------------------------------------------

ImageView::ImageView(View* InParent, Bitmap* InBitmap)
    : View(InParent,
        0,
        0,
        InParent ? InParent->Width() : 0,
        InParent ? InParent->Height() : 0)
    , Image(InBitmap)
    , Blend(Video::BLEND_SOLID)
{
    ImageW = 0;
    ImageH = 0;

    if (Image)
    {
        ImageW = Image->Width();
        ImageH = Image->Height();
    }

    XOffset = 0;
    YOffset = 0;

    // Center in parent view
    const int ParentW = InParent ? InParent->Width() : 0;
    const int ParentH = InParent ? InParent->Height() : 0;

    if (ImageW > 0 && ParentW > ImageW) XOffset = (ParentW - ImageW) / 2;
    if (ImageH > 0 && ParentH > ImageH) YOffset = (ParentH - ImageH) / 2;
}

ImageView::~ImageView()
{
}

// --------------------------------------------------------------------

void ImageView::Refresh()
{
    if (!Image || ImageW <= 0 || ImageH <= 0)
        return;

    DrawBitmap(
        XOffset,
        YOffset,
        XOffset + ImageW,
        YOffset + ImageH,
        Image,
        Blend
    );
}

// --------------------------------------------------------------------

void ImageView::SetPicture(Bitmap* InBmp)
{
    Image = InBmp;

    ImageW = 0;
    ImageH = 0;
    XOffset = 0;
    YOffset = 0;

    if (Image)
    {
        ImageW = Image->Width();
        ImageH = Image->Height();
    }

    // Center in THIS view
    const int VW = View::Width();
    const int VH = View::Height();

    if (ImageW > 0 && VW > ImageW) XOffset = (VW - ImageW) / 2;
    if (ImageH > 0 && VH > ImageH) YOffset = (VH - ImageH) / 2;
}
