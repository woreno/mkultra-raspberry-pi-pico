#define SHAPES_TY W8

struct Sz
{
	union
	{
		SHAPES_TY data[2]; 
		struct { SHAPES_TY lx,ly; };
		struct { SHAPES_TY width,height; };
		struct { SHAPES_TY w,h; };
	};
	Sz(W16 lx_, W16 ly_)
	{
		lx = lx_;
		ly = ly_;
	}
	inline W16 getArea()
	{
		return lx*ly;
	}
};
template<typename T_>
struct Pt_
{
	union
	{
		T_ data[2]; 
		struct { T_ x0,y0; };
		struct { T_ x, y; };
	};

	Pt_(T_ x_, T_ y_) { x=x_; y=y_; }
};
typedef Pt_<SHAPES_TY> Pt;
typedef Pt_<FP>  Pt2f;

struct Ret
{
	union
	{
		SHAPES_TY data[2], ldata[2]; 
		struct { SHAPES_TY x0,y0,width,height; };
		struct { SHAPES_TY x,y,lx,ly; };
	};

	Ret()
	{
		x = y = width = height = 0;
	}

	Ret(const SHAPES_TY x0_, const SHAPES_TY y0_, const SHAPES_TY w_, const SHAPES_TY h_)
	{
		x0 = x0_;
		y0 = y0_;
		width = w_;
		height = h_;
	}
	W1 hasNoSize()  const { return (lx==0)||(ly==0); }

	Pt bl()     const { return Pt(data[0]                   , data[1] + (ldata[1])      ); }
	/*
	Pt tl()     const { return Pt(data[0]                   , data[1]                   ); }
	Pt et()     const { return Pt(data[0] + (ldata[0])/2    , data[1]                   ); }
	Pt tr()     const { return Pt(data[0] + (ldata[0])      , data[1]                   ); }
	Pt er()     const { return Pt(data[0] + (ldata[0])      , data[1] + (ldata[1])/2    ); }
	Pt br()     const { return Pt(data[0] + (ldata[0])      , data[1] + (ldata[1])      ); }
	Pt eb()     const { return Pt(data[0] + (ldata[0])/2    , data[1] + (ldata[1])      ); }
	Pt el()     const { return Pt(data[0]                   , data[1] + (ldata[1])/2    ); }
	Pt center() const { return Pt(data[0] + (ldata[0])/2    , data[1] + (ldata[1])/2    ); }*/
	SHAPES_TY x1()	   const { return  x0+width; } 
	SHAPES_TY y1()	   const { return  y0+height; }
	 
	W0 expand(SW16 border_)
	{
		const SW16 b2 = 2*border_;
		x-=border_;
		y-=border_;
		lx+=b2;
		ly+=b2;
	}
		/*
	W0 getPointsArr(Pt* arr_) const 
	{
		arr_[0] = tl();
		arr_[1] = tr();
		arr_[2] = br();
		arr_[3] = bl();
	}
	W0 scale(Pt pivot__, FP scX_,  FP scY_)
	{
		Pt2f pivot_(pivot__);

		Pt2f dtl_ = tl() - pivot_;
		dtl_.data[0] = (float)dtl_.x*scX_;
		dtl_.data[1] = (float)dtl_.y*scY_;
		
		Pt2f dbr_ = br() - pivot_;
		dbr_.data[0] = (float)dbr_.x*scX_;
		dbr_.data[1] = (float)dbr_.y*scY_;

		Pt2f tl_ = pivot_ + dtl_, 
				    br_ = pivot_ + dbr_;

		data[0]=tl_.data[0];
		data[1]=tl_.data[1];
		ldata[0]=br_.data[0] - tl_.data[0];
		ldata[1]=br_.data[1] - tl_.data[1];
	}

	W0 operator+=(const Pt p_)
	{
		r_.x+=p_.x, r_.y+=y;
	}
	W0 operator-=(const Pt p_)
	{
		r_.x-=p_.x, r_.y-=y;
	}
	W0 operator+=(const Sz s_)
	{
		r_.x+=p_.lx, r_.y+=ly;
	}
	W0 operator-=(const Sz s_)
	{
		r_.x-=p_.lx, r_.y-=ly;
	}
	*/
};
/*
Ret operator+(const Ret r_, const Pt p_)
{
	return Ret(r_.x+p_.x, 
                     r_.y+p_.y, 
                     r_.lx, r_.ly);
}
Ret operator-(const Ret r_, const Pt p_)
{
	return Ret(r_.x - p_.x, 
               r_.y - p_.y, 
               r_.lx, 
			   r_.ly);
}
Ret operator-(const Ret r_, const Sz s_)
{
	return Ret(r_.x-s_.lx, 
               r_.y-s_.ly, 
               r_.lx, 
			   r_.ly);
}
Ret operator+(const Ret r_, const Sz s_)
{
	return Ret(r_.x+s_.lx, 
               r_.y+s_.ly, 
               r_.lx, 
			   r_.ly);
}
*/
  

#define DISP_COLOR_R(X_)			((X_)&0xF)
#define DISP_COLOR_G(X_)			(((X_)>>4)&0xF)
#define DISP_COLOR_B(X_)			(((X_)>>8)&0xF)

#define Color_Blue      ((W16)(DISP_COLOR_MK_W16(0,0,13)))
#define Color_Red       ((W16)(DISP_COLOR_MK_W16(15,0,0)))
#define Color_White     ((W16)(DISP_COLOR_MK_W16(15,15,15)))
#define Color_GrayLight ((W16)(DISP_COLOR_MK_W16(13,13,13)))
#define Color_Gray      ((W16)(DISP_COLOR_MK_W16(10,10,10)))
#define Color_Green     ((W16)(DISP_COLOR_MK_W16(0,14,0)))
#define Color_Greenish  ((W16)(DISP_COLOR_MK_W16(0,9,0)))
#define Color_Blueish   ((W16)(DISP_COLOR_MK_W16(7,7,15)))
#define Color_Black     ((W16)(DISP_COLOR_MK_W16(0,0,0)))