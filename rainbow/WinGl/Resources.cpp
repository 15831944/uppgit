#include "WinGl.h"
#include <plugin/png/png.h>
#ifdef GUI_WINGL

NAMESPACE_UPP

Resources resources;

Resources::Resources() : autoAtlas(1024, 1024)
{
	px = 0;
	py = 0;
	maxh = 0;
	currentSerialId = -1;
	bindedTextures = 0;
}

void Resources::SetTextureFiltring(int opts)
{
	if(opts & LINEAR_FILTERING)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else if(opts & NEAREST_FILTERING)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
}

const Texture& Resources::CreateTexture(const Image& img, int width, int height)
{
	int64 serialId = img.GetSerialId();
	Texture& t = textures.Add(serialId);
	t.serialId = serialId;
	t.atlasSerialId = -1;
	t.width = width > 0 ? width : img.GetWidth();
	t.height = height > 0 ? height : img.GetHeight();
	t.realWidth = t.width;
	t.realHeight = t.height;
	t.x = 0;
	t.y = 0;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.width, t.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, img);
	++bindedTextures;
	return t;
}

void Resources::CreateSubTexture(const Texture& t, const Image& img, int x, int y)
{
	Bind(autoAtlas.serialId, 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, img.GetWidth(), img.GetHeight(), GL_BGRA, GL_UNSIGNED_BYTE, img);
}

 const Texture& Resources::Bind(const Image& img, int opts)
{
	int64 serialId = img.GetSerialId();

	const Texture* texture = textures.FindPtr(serialId);
	if(texture)
	{
		if(texture->atlasSerialId >= 0)
			serialId = texture->atlasSerialId;
	}
	else if(opts & AUTO_ATLAS)
	{
		int w = img.GetWidth();
		int h = img.GetHeight();
		
		if(w <= autoAtlas.width && h <= autoAtlas.height && w <= 64 && h <= 64)
		{
			bool add = px + w <= autoAtlas.width && py + h <= autoAtlas.height;
			
			if(!add)
			{
				if(py + maxh + h > autoAtlas.height)
				{
					add = false;
				}
				else
				{
					px = 0;
					py += maxh;
					maxh = 0;
					add = true;
				}
			}
			
			if(add)
			{
				if(h > maxh)
					maxh = h;
				
				Texture& t = textures.Add(img.GetSerialId());
				t.atlasSerialId = autoAtlas.serialId;
				t.width = w;
				t.height = h;
				t.realWidth = autoAtlas.width;
				t.realHeight = autoAtlas.height;
				t.x = px;
				t.y = py;
				
				CreateSubTexture(autoAtlas, img, px, py);
				Copy(autoAtlasImage, Point(px, py), img, img.GetSize());
				
				px += w;
				
				texture = &t;
				serialId = autoAtlas.serialId;
			}
		}
	}
	
	if(!Bind(serialId, opts))
		return *texture;

	if(!texture)
	{
		texture = &CreateTexture(img);
	}

	return *texture;
}

bool Resources::Bind(int64 serialId, int opts)
{
	if(!(opts & FORCE_BIND) && serialId == currentSerialId)
	{
		if(opts & FORCE_FILTERING)
			SetTextureFiltring(opts);
		return false;
	}
	
	currentSerialId = serialId;
	glBindTexture(GL_TEXTURE_2D, (GLuint) serialId);
	SetTextureFiltring(opts);
	return true;
}

void Resources::BindStatic()
{
	if(autoAtlas.serialId < 0)
	{
		autoAtlasBuffer.Create(autoAtlas.width, autoAtlas.height);
		Fill(autoAtlasBuffer, RGBAZero(), autoAtlasBuffer.GetLength());
		autoAtlasImage = autoAtlasBuffer;
		Bind(autoAtlasImage.GetSerialId());		
		autoAtlas = CreateTexture(autoAtlasImage);
	}
	
	if(!staticImages.IsEmpty())
	{
		for(int i = 0; i < staticImages.GetCount(); i++)
			Bind(staticImages[i].img, staticImages[i].linear ? Resources::LINEAR_FILTERING : Resources::NEAREST_FILTERING);
		staticImages.Clear();
	}
	
	if(!staticAtlases.IsEmpty())
	{
		for(int i = 0; i < staticAtlases.GetCount(); i++)
			Bind(staticAtlases[i].Make(textures), staticAtlases[i].linear ? Resources::LINEAR_FILTERING : Resources::NEAREST_FILTERING);
		staticAtlases.Clear();
	}
	
	if(!staticFonts.IsEmpty())
	{
		for(int i = 0; i < staticFonts.GetCount(); i++)
			GetFont(staticFonts[i], true);
		staticFonts.Clear();
	}	
}

void Resources::Add(const Image& img, bool linear)
{
	ImageResource& ir = staticImages.Add();
	ir.img = img;
	ir.linear = linear;
}

void Resources::Add(const Font& fnt)
{
	Font& f = staticFonts.Add(fnt);
}

void Resources::Add(Iml* images, bool linear)
{
	for(int i = 0; i < images->GetCount(); i++)
		Add(images->Get(i), linear);
}

void Resources::AddAtlas(const char* atlasName, const Image& img)
{
	Atlas& atlas = staticAtlases.GetAdd(atlasName);
	atlas.linear = false;
	atlas.parts.Add(img);
}

void Resources::AddAtlas(const char* atlasName, Iml* images)
{
	Atlas& atlas = staticAtlases.GetAdd(atlasName);
	atlas.linear = false;
	for(int i = 0; i < images->GetCount(); i++)
		atlas.parts.Add(images->Get(i));
}

Image Atlas::Make(ArrayMap<int64, Texture>& textures)
{
	width = 512;
	height = 512;

	ImageBuffer ib(width, height);
	Fill(ib, RGBAZero(), ib.GetLength());

	Point p(0, 0);
	int maxh = 0;
	
	int tidx = textures.GetCount();
	
	for(int i = 0; i < parts.GetCount(); i++)
	{
		Image& img = parts[i];
		
		int w = img.GetWidth();
		int h = img.GetHeight();
		
		if(w > width || h > height)
			continue;
		
		if(p.x + w > width)
		{
			if(p.y + maxh + h > height)
				continue;

			p.x = 0;
			p.y += maxh;
			maxh = 0;
		}
			
		if(h > maxh)
			maxh = h;
		
		Texture& t = textures.Add(img.GetSerialId());
		t.width = w;
		t.height = h;
		t.realWidth = width;
		t.realHeight = height;
		t.x = p.x;
		t.y = p.y;
		
		Copy(ib, p, img, img.GetSize());
		
		p.x += w;
	}

	Image finalImg(ib);
	int64 finalSerialId = finalImg.GetSerialId();
	
	for(int i = tidx; i < textures.GetCount(); i++)
		textures[i].atlasSerialId = finalSerialId;
	
	return finalImg;	
}

OpenGLFont& Resources::GetFont(const char* fontName, int fontHeight, bool preload, const byte* fontDef,  const byte** imagesData, const int* imagesSize, int imagesCount)
{
	int n = fonts.Find(fontName);
	OpenGLFont* fgl = NULL;
	if(n >= 0)
		fgl = &fonts[n];
	else
	{
		fgl = &fonts.Add(fontName);
		fgl->preload = preload;
		
		if(fontDef)
			fgl->LoadBrc(fontDef, imagesData, imagesSize, imagesCount);
		else
			fgl->Load(fontName);
	}
	
	fgl->scale = (fontHeight * 96.f / 72.f) / 72.f;
	return *fgl;
}

OpenGLFont& Resources::GetFont(const Font& font, bool preload)
{
	if(font.IsBold())
		return GetFont("TahomaB", font.GetHeight(), preload,
			tahomaBFontDef, (const byte**) tahomaBFontImg, tahomaBFontImg_length, tahomaBFontImg_count);
	else
		return GetFont("TahomaN", font.GetHeight(), preload,
			tahomaNFontDef, (const byte**) tahomaNFontImg, tahomaNFontImg_length, tahomaNFontImg_count);
}

bool Fbo::Create(int width, int height, bool resize)
{
	this->width = width;
	this->height = height;

	if(!resize)
	{
		glGenFramebuffers(1, &fbId);
		glGenRenderbuffers(1, &rbId);
		glGenTextures(1, &texId);
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	
	glBindRenderbuffer(GL_RENDERBUFFER, rbId);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbId, 0);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbId);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbId);
	status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		
	ready = true;
	
	return status == GL_FRAMEBUFFER_COMPLETE;
}

void Fbo::Resize(int width, int height)
{
	if(ready)
		Create(width, height, true);
}

void Fbo::Bind()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbId);
}

void Fbo::Unbind()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Fbo::Clear()
{
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void Fbo::BlitToScreen()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void Fbo::BlitTo(const Fbo& fbo)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo.fbId);
	glBlitFramebuffer(0, 0, width, height, 0, 0, fbo.width, fbo.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

String Fbo::GetError()
{
	switch(status)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		return "Framebuffer complete";
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		return "Framebuffer incomplete attachment";
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		return "Framebuffer incomplete missing attachment";
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		return "Framebuffer incomplete draw buffer";
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		return "Framebuffer incomplete read buffer";
	case GL_FRAMEBUFFER_UNSUPPORTED:
		return "Framebuffer unsupported";
	default:
		return "Unknown error";
	}
}

int64 GetHighTickCount()
{
	static int64 counterFreq = 0;
	if(counterFreq == 0)
	{
		if(!QueryPerformanceFrequency((LARGE_INTEGER*) &counterFreq))
			counterFreq = 0;
	}
	if(counterFreq > 0)
	{
		int64 counter;
		QueryPerformanceCounter((LARGE_INTEGER*) &counter);
		return counter * 1000 / counterFreq;
	}
	else
	{
		return GetTickCount();
	}
}

float GetFps()
{
	static float fps = 0.0f;
	static int64 updateInterval = 1000;
	static int64 timeSinceLastUpdate = 0;
	static int64 frameCount = 0;
	static int64 currentTick;
	static int64 lastTick;
	static bool  isFirst = true;

	if(isFirst)
	{
		currentTick = frameInfo.curr_tick_count;
		lastTick = currentTick;
		isFirst = false;
	}

	frameCount++;
	currentTick = frameInfo.curr_tick_count;

	int64 elapsed = currentTick - lastTick;

	lastTick = currentTick;
	timeSinceLastUpdate += elapsed;

	if (timeSinceLastUpdate > updateInterval)
	{
		if (timeSinceLastUpdate)
		{
			fps = (frameCount / float(timeSinceLastUpdate)) * 1000.f;
			frameCount = 0;
			timeSinceLastUpdate -= updateInterval;
		}
	}

	return fps;
}

#endif
END_UPP_NAMESPACE