#include <Windows.h>
#include <gdiplus.h>
#pragma comment(lib,"gdiplus")
using namespace Gdiplus;
#include <stdexcept>
using std::runtime_error;

struct GdiplusInit {
	GdiplusInit() {
		GdiplusStartupInput inp;
		GdiplusStartupOutput outp;
		if (Ok != GdiplusStartup(&token_, &inp, &outp))
			throw runtime_error("GdiplusStartup");
	}
	~GdiplusInit() {
		GdiplusShutdown(token_);
	}
private:
	ULONG_PTR token_;
};

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

void drawrect(Gdiplus::Graphics* g)
{
	g->SetCompositingMode(CompositingModeSourceCopy);
	g->SetCompositingQuality(CompositingQualityHighSpeed);
	g->SetPixelOffsetMode(PixelOffsetModeNone);
	g->SetSmoothingMode(SmoothingModeNone);
	g->SetInterpolationMode(InterpolationModeDefault);

	g->Clear(Color(255, 255, 255, 255));

	Pen gdsPen(Color::Black, 5);
	g->DrawRectangle(&gdsPen, 100, 100, 300, 200);
}

HBITMAP CreateGreyscaleBitmap(int cx, int cy, int bpp)
{
	UINT ncols = (UINT)1 << bpp;
	//BITMAPINFO* pbmi = (BITMAPINFO*)alloca(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256);
	//BITMAPINFO* pbmi = (BITMAPINFO*)alloca(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * ncols);
	BITMAPINFO* pbmi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * ncols);
	pbmi->bmiHeader.biSize = sizeof(pbmi->bmiHeader);
	pbmi->bmiHeader.biWidth = cx;
	pbmi->bmiHeader.biHeight = cy;
	pbmi->bmiHeader.biPlanes = 1;
	pbmi->bmiHeader.biBitCount = bpp;
	pbmi->bmiHeader.biCompression = BI_RGB;
	pbmi->bmiHeader.biSizeImage = 0;
	pbmi->bmiHeader.biXPelsPerMeter = 14173;
	pbmi->bmiHeader.biYPelsPerMeter = 14173;
	pbmi->bmiHeader.biClrUsed = 0;
	pbmi->bmiHeader.biClrImportant = 0;

	if (bpp == 1) {
		pbmi->bmiColors[0].rgbRed = 0;
		pbmi->bmiColors[0].rgbGreen = 0;
		pbmi->bmiColors[0].rgbBlue = 0;
		pbmi->bmiColors[0].rgbReserved = 0;

		pbmi->bmiColors[1].rgbRed = 255;
		pbmi->bmiColors[1].rgbGreen = 255;
		pbmi->bmiColors[1].rgbBlue = 255;
		pbmi->bmiColors[1].rgbReserved = 0;
	}
	else {
		for (int i = 0; i < ncols; i++)
		{
			pbmi->bmiColors[i].rgbRed = i;
			pbmi->bmiColors[i].rgbGreen = i;
			pbmi->bmiColors[i].rgbBlue = i;
			pbmi->bmiColors[i].rgbReserved = 0;
		}
	}

	PVOID pv;
	return CreateDIBSection(NULL, pbmi, DIB_RGB_COLORS, &pv, NULL, 0);
}

Bitmap* CopyTo8BitGray(Bitmap* b)
{
	UINT w = b->GetWidth(), h = b->GetHeight();
	HBITMAP hbm; // original
	b->GetHBITMAP(Color::White, &hbm);

	HBITMAP hbm0 = CreateGreyscaleBitmap(w,h,8); // target 8bit grayscale

	HDC sdc = GetDC(NULL);

	HDC hdc = CreateCompatibleDC(sdc);
	SelectObject(hdc, hbm); // origianl obj

	HDC hdc0 = CreateCompatibleDC(sdc);
	SelectObject(hdc0, hbm0); // target obj

	BitBlt(hdc0, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
	Bitmap* b0 = Bitmap::FromHBITMAP(hbm0, NULL);

	DeleteDC(hdc);
	DeleteDC(hdc0);
	ReleaseDC(NULL, sdc);
	DeleteObject(hbm);
	DeleteObject(hbm0);

	return b0;
}

void createbmp(int nw,int nh)
{
	//BYTE* pImageData = new BYTE[nw * 4 * nh];
	//Bitmap bmp(nw, nh, 4 * nw, PixelFormat1bppIndexed, pImageData);
	//Bitmap* bmp = new Bitmap(nw, nh, 4 * nw, PixelFormat32bppARGB, pImageData);
	Bitmap* bmp = new Bitmap(nw, nh, PixelFormat32bppARGB);
	
	Gdiplus::Graphics* g = Gdiplus::Graphics::FromImage(bmp);
	
	drawrect(g);

	CLSID imgClsid;
	GetEncoderClsid(L"image/bmp", &imgClsid);

	bmp->Save(L"test.bmp", &imgClsid, NULL);

	//Bitmap* bmp2 = CopyToBpp(bmp, 8);
	Bitmap* bmp2 = CopyTo8BitGray(bmp);
	bmp2->Save(L"test2.bmp", &imgClsid, NULL);

	delete g;
	delete bmp;
	//delete pImageData;
}

void bmptest(int w, int h)
{
	//HBITMAP hbm0 = CreateGreyscaleBitmap(w, h, 8);
	Bitmap* pbmp = Bitmap::FromHBITMAP(CreateGreyscaleBitmap(w, h, 8), NULL);

	Gdiplus::Graphics* g = Gdiplus::Graphics::FromImage(pbmp);

	drawrect(g);

	CLSID imgClsid;
	GetEncoderClsid(L"image/bmp", &imgClsid);

	pbmp->Save(L"test3.bmp", &imgClsid, NULL);

	delete g;
	delete pbmp;
}

int main(int argc, char* argv[]) 
{
	GdiplusInit gdiplusinit;
	
	//createbmp(800, 600);
	bmptest(800, 600);


	return 0;
}