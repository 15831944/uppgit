#include "ScanLine.h"

#ifdef RASTERIZER2

void Rasterizer::Init()
{
	x1 = y1 = 0;
	min_y = INT_MAX;
	max_y = INT_MIN;
}

void Rasterizer::Reset()
{
	PAINTER_TIMING("Rasterizer::Reset");
	Init();
	for(int i = 0; i < sz.cy; i++)
		cell[i].SetCount(0);
}

void Rasterizer::SetClip(const Rectf& rect)
{
	cliprect = rect & Sizef(sz);
}

Rasterizer::Rasterizer(int cx, int cy)
{
	sz.cx = cx;
	sz.cy = cy;
	cell.Alloc(sz.cy);
	ymax = (sz.cy << 8) - 1;
	xmax = (sz.cx << 8) - 1;
	cliprect = Sizef(sz);
	Reset();
}

inline Rasterizer::Cell *Rasterizer::AddCells(int y, int n)
{
	Vector<Cell>& v = cell[y];
	if(v.GetCount() == 0) {
		v.Reserve(16);
		v.SetCount(n);
		return &v[0];
	}
	y = v.GetCount();
	v.SetCount(y + n);
	return &v[y];
}

inline void Rasterizer::RenderHLine(int ey, int x1, int y1, int x2, int y2)
{
	int ex1 = x1 >> 8;
	int ex2 = x2 >> 8;
	int fx1 = x1 & 255;
	int fx2 = x2 & 255;
	Cell *c;
	if(y1 == y2)
		return;
	if(ex1 == ex2) {
		int delta = y2 - y1;
		c = AddCells(ey, 1);
		c->x = ex1;
		c->cover = y2 - y1;
		c->area = (fx1 + fx2) * delta;
		return;
	}
	int p = (256 - fx1) * (y2 - y1);
	int first = 256;
	int incr  = 1;
	int dx = x2 - x1;
	if(dx < 0) {
		p = fx1 * (y2 - y1);
		first = 0;
		incr = -1;
		dx = -dx;
	}
	int delta = p / dx;
	int mod = p % dx;
	if(mod < 0) {
		delta--;
		mod += dx;
	}
	c = AddCells(ey, abs(ex2 - ex1) + 1);
	c->x = ex1;
	c->cover = delta;
	c->area = (fx1 + first) * delta;
	c++;
	ex1 += incr;
	y1 += delta;
	if(ex1 != ex2) {
		p = (y2 - y1 + delta) * 256;
		int lift = p / dx;
		int rem = p % dx;
		if (rem < 0) {
			lift--;
			rem += dx;
		}
		mod -= dx;
		while(ex1 != ex2) {
			delta = lift;
			mod += rem;
			if(mod >= 0) {
				mod -= dx;
				delta++;
			}
			c->x = ex1;
			c->cover = delta;
			c->area = delta * 256;
			c++;
			y1 += delta;
			ex1 += incr;
		}
	}
	delta = y2 - y1;
	c->x = ex2;
	c->cover = delta;
	c->area = (fx2 + 256 - first) * delta;
}

void Rasterizer::LineRaw(int x1, int y1, int x2, int y2)
{
//	PAINTER_TIMING("LineRaw");
	enum dx_limit_e { dx_limit = 16384 << 8 };
	int dx = x2 - x1;
	if(dx >= dx_limit || dx <= -dx_limit) {
		int cx = (x1 + x2) >> 1;
		int cy = (y1 + y2) >> 1;
		LineRaw(x1, y1, cx, cy);
		LineRaw(cx, cy, x2, y2);
		return;
    }
	int dy = y2 - y1;
	int ex1 = x1 >> 8;
	int ex2 = x2 >> 8;
	int ey1 = y1 >> 8;
	int ey2 = y2 >> 8;
	int fy1 = y1 & 255;
	int fy2 = y2 & 255;
	
	ASSERT(ey1 >= 0 && ey1 < sz.cy && ey2 >= 0 && ey2 < sz.cy);

	Cell *c;
	int x_from, x_to;
	int p, rem, mod, lift, delta, first, incr;

	if(ey1 < min_y) min_y = ey1;
	if(ey1 > max_y && ey1 < sz.cy) max_y = ey1;
	if(ey2 < min_y) min_y = ey2;
	if(ey2 > max_y && ey2 < sz.cy) max_y = ey2;

	if(ey1 == ey2) {
		RenderHLine(ey1, x1, fy1, x2, fy2);
		return;
    }
	incr  = 1;
	if(dx == 0) {
		int ex = x1 >> 8;
		int two_fx = (x1 - (ex << 8)) << 1;
		int area;
		first = 256;
		if(dy < 0) {
			first = 0;
			incr  = -1;
		}
		x_from = x1;
		delta = first - fy1;
		c = AddCells(ey1, 1);
		c->x = ex1;
		c->cover = delta;
		c->area = two_fx * delta;
		ey1 += incr;
		delta = first + first - 256;
		area = two_fx * delta;
		while(ey1 != ey2) {
			c = AddCells(ey1, 1);
			c->x = ex1;
			c->cover = delta;
			c->area = area;
			ey1 += incr;
		}
		delta = fy2 - 256 + first;
		c = AddCells(ey1, 1);
		c->x = ex1;
		c->cover = delta;
		c->area = two_fx * delta;
		return;
    }
	p = (256 - fy1) * dx;
	first = 256;
	if(dy < 0) {
		p = fy1 * dx;
		first = 0;
		incr = -1;
		dy = -dy;
    }
	delta = p / dy;
	mod = p % dy;
	if(mod < 0) {
		delta--;
		mod += dy;
	}
	x_from = x1 + delta;
	RenderHLine(ey1, x1, fy1, x_from, first);
	ey1 += incr;
	if(ey1 != ey2) {
		p = dx << 8;
		lift = p / dy;
		rem = p % dy;
		if(rem < 0) {
			lift--;
			rem += dy;
		}
		mod -= dy;
		while(ey1 != ey2) {
			delta = lift;
			mod  += rem;
			if(mod >= 0) {
				mod -= dy;
				delta++;
			}
			x_to = x_from + delta;
			RenderHLine(ey1, x_from, 256 - first, x_to, first);
			x_from = x_to;
			ey1 += incr;
		}
    }
	RenderHLine(ey1, x_from, 256 - first, x2, fy2);
}

bool Rasterizer::BeginRender(int y, const Cell *&c, const Cell *&e)
{
	if(y < min_y || y > max_y) return false;
	Vector<Cell>& cl = cell[y];
	if(cl.GetCount() == 0) return false;
	Sort(cl);
	c = cl;
	e = cl.End();
	return true;
}

#endif
