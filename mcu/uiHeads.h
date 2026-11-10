// não mudar
#define MCU_UI


// UI_PAGE_SOURCE_LIST_DO_BEFORE_ACCESS
// UI_PAGE_SOURCE_LIST_DO_AFTER_ACCESS

#include <cfloat>
#include <vector>
#include <string>
#define UI_MENU_SIZE 3
#define UI_ERROR_ALLOC 501
#define ICON_DEFAULT (0xFF)

#define DEBUG_UI(X_,...) // mcu_log(X_,__VA_ARGS__);

class UI_PAGE;
class UI_PAGE_TEXT;
class UI_PAGE_OPTIONS;
class UI_PAGE_SOURCE_LIST;
UI_PAGE    *uiCurrPage;


W0 animatePage(SW8 dir_);

//estado
enum UI_BUTTONS { UI_BUTTON_UP    = 0,
	              UI_BUTTON_LEFT  = 1,
				  UI_BUTTON_RIGHT = 2, 
				  UI_BUTTON_DOWN  = 3, 
				  UI_BUTTON_ENTER = 4,
				  UI_BUTTON_ROTATE_UP = 5,
				  UI_BUTTON_ROTATE_DOWN = 6,
				  UI_BUTTON_ESC   = 7,
				  UI_BUTTON_NONE  = 8};

UI_BUTTONS uiButton = UI_BUTTON_NONE;

	
	#define UI_ITEM_VALUE_BOOL				0
	#define UI_ITEM_VALUE_W8_RANGE_ENUM		1
	#define UI_ITEM_VALUE_SW8_RANGE_ENUM	2
	#define UI_ITEM_VALUE_W16_RANGE_ENUM	3
	#define UI_ITEM_VALUE_SW16_RANGE_ENUM	4
	#define UI_ITEM_VALUE_W32_RANGE_ENUM	5
	#define UI_ITEM_VALUE_SW32_RANGE_ENUM	6
	#define UI_ITEM_VALUE_FLT32_RANGE_ENUM	7
	#define UI_ITEM_VALUE_STRING_ENUM		8
	#define UI_ITEM_VALUE_STRING			9
	#define UI_ITEM_VALUE_W16_ENUM			10
	#define UI_ITEM_VALUE_CALLBACK			11
	#define UI_ITEM_VALUE_UIPAGE			12 


typedef W0(*UI_CALLBACK)(W0*);

class UI_ITEM_VALUE
{
public:
	W8 type;
	W0 *valuePtr;
	const SW8* prefixStr, *sufixStr;
	union
	{
		struct { UI_PAGE* page; } page; 
		struct { UI_CALLBACK call; W0* args; } callback; 
		struct {  W8 start,   delta, end;   } W8range;
		struct {  SW8 start,  delta, end;  } SW8range;
		struct {  W16 start,  delta, end;  } W16range;
		struct {  SW16 start, delta, end; } SW16range;
		struct {  W32 start,  delta, end;  } W32range;
		struct {  SW32 start, delta, end; } SW32range;
		struct {  FP32 start, delta, end; } FP32range;
		struct { W8 start, end;   const SW8** strsPtr;  } strEnum;
		struct { W8 start, end;   const W16* valsPtr;  } W16Enum;
	};

	UI_ITEM_VALUE()
	{}
	UI_ITEM_VALUE(SW8* ptr_)
	{
		type = UI_ITEM_VALUE_STRING;
		valuePtr = ptr_;
	}
	UI_ITEM_VALUE(W8* ptr_, const W16* valsPtr_, W8 start_, W8 end_, const SW8* sufixStr_=0): type(UI_ITEM_VALUE_W16_ENUM), valuePtr(ptr_)
	{
		W16Enum.valsPtr = valsPtr_;
		W16Enum.start =start_;
		W16Enum.end = end_;
		prefixStr = 0;
		sufixStr = sufixStr_;
	}
	UI_ITEM_VALUE(W8* ptr_, const SW8** strsPtr_, W8 start_, W8 end_, const SW8* sufixStr_=0): type(UI_ITEM_VALUE_STRING_ENUM), valuePtr(ptr_)
	{
		strEnum.strsPtr = strsPtr_;
		strEnum.start =start_;
		strEnum.end = end_;
		prefixStr = 0;
		sufixStr = sufixStr_;
	}
	UI_ITEM_VALUE(W1* ptr_): type(UI_ITEM_VALUE_BOOL), valuePtr(ptr_)
	{
		prefixStr = 0;
		sufixStr = 0;
	}
	UI_ITEM_VALUE(W8 type_, W0* ptr_): type(type_), valuePtr(ptr_)
	{
		prefixStr = 0;
		sufixStr = 0;
	}
		#define UI_CASE_ADD(NAME_,  KIND_)\
			case NAME_:								\
			{										\
				KIND_ val = *(KIND_*)valuePtr;			\
				val = val + ((KIND_)sign_)* KIND_##range.delta;	\
				if (val > KIND_##range.end)			\
					val = KIND_##range.end;			\
				else if (val < KIND_##range.start)	\
					val = KIND_##range.start;		\
				*(KIND_*)valuePtr = val;			\
			}										\
			break;

	W0 inc(const SW8 sign_=+1)
	{
		switch(type)
		{
			case UI_ITEM_VALUE_BOOL:
			{
				 *((W1*)valuePtr)=!( *((W1*)valuePtr));
			}
			break;
			UI_CASE_ADD(UI_ITEM_VALUE_W8_RANGE_ENUM,     W8)
			UI_CASE_ADD(UI_ITEM_VALUE_SW8_RANGE_ENUM,    SW8)
			UI_CASE_ADD(UI_ITEM_VALUE_W16_RANGE_ENUM,    W16)
			UI_CASE_ADD(UI_ITEM_VALUE_SW16_RANGE_ENUM  , SW16)
			UI_CASE_ADD(UI_ITEM_VALUE_W32_RANGE_ENUM,    W32)
			UI_CASE_ADD(UI_ITEM_VALUE_SW32_RANGE_ENUM,   SW32)
			UI_CASE_ADD(UI_ITEM_VALUE_FLT32_RANGE_ENUM,  FP32)
			case UI_ITEM_VALUE_W16_ENUM:
			case UI_ITEM_VALUE_STRING_ENUM:
			{	
				SW8 curr = *((W8*)valuePtr); 
				curr+=sign_;
				if (curr>strEnum.end)
					curr=strEnum.end;
				else if (curr<strEnum.start) 
					curr=strEnum.start;
				*((W8*)valuePtr) = curr; 

			} 
			break;
		};
	}
	#undef UI_CASE_ADD
	
	W0 getValueStr(SW8* str_)
	{
		*str_=0;
		const SW8 uStr[]   ="%u";
		const SW8 iStr[]   ="%i";
		const SW8 fpStr[]  ="%3.2f";

		#define UI_CASE_SPRINT(NAME_, KIND_, SPR_TY_)\
			case NAME_:								 \
			{										 \
				KIND_ val = *(KIND_*)valuePtr;		 \
				sprintf(str_, SPR_TY_, val );		 \
			}										 \
			break;

		switch(type)
		{
			case UI_ITEM_VALUE_BOOL:
			{
				str_[0]='O';
				if ( *((W1*)valuePtr) )
				{
					str_[1]='N';
					str_[2]=0;
				}
				else
				{
					str_[2]=str_[1]='F';
					str_[3]=0;
				}
			}
			break; 
			UI_CASE_SPRINT(UI_ITEM_VALUE_W8_RANGE_ENUM,     W8,    uStr)
			//UI_CASE_SPRINT(UI_ITEM_VALUE_SW8_RANGE_ENUM,    SW8,   iStr)
			UI_CASE_SPRINT(UI_ITEM_VALUE_W16_RANGE_ENUM,    W16,   uStr)
			//UI_CASE_SPRINT(UI_ITEM_VALUE_SW16_RANGE_ENUM,   SW16,  iStr)
			//UI_CASE_SPRINT(UI_ITEM_VALUE_W32_RANGE_ENUM,    W32,   uStr)
			//UI_CASE_SPRINT(UI_ITEM_VALUE_SW32_RANGE_ENUM,   SW32,  iStr)
			UI_CASE_SPRINT(UI_ITEM_VALUE_FLT32_RANGE_ENUM,  FP32,  fpStr)
			
			case UI_ITEM_VALUE_W16_ENUM:
			{
				W8 index = *((W8*)valuePtr);
				W16* arr= ((W16*)W16Enum.valsPtr);
				sprintf(str_, uStr, arr[index]);
				//printf(str_);
			}
			break;
			case UI_ITEM_VALUE_STRING_ENUM:
			{
				W8 index = *((W8*)valuePtr);
				const SW8 **strs= ((const SW8**)strEnum.strsPtr);
				strcpy(str_, strs[index]);
			}
			break;
			case UI_ITEM_VALUE_STRING:
			{
				SW8 *str= ((SW8*)valuePtr);
				strcpy(str_, str);
			}
			break;
		};

		#undef UI_CASE_SPRINT
	}
	  
};
// *** Representa uma linha de texto, com um icone e outros dados 
class UI_ITEM
{
	#define UI_ITEM_NAME_ALLOCATED  7
	#define UI_ITEM_HAS_CONTENTS	6
	#define UI_ITEM_ICON_ST			0
	#define UI_ITEM_ICON_END		5

public:
	UI_ITEM_VALUE *value;
	union
	{
		struct {
					SW8* name;
					W32  itemData32;
					W8   info;
				};
		struct {
					SW8* bufferStart;
					W16  lineLength;
				};
	};
	virtual SW8* getName()
	{
		return name;
	}
	W1   isNameAllocated() const
	{
		return W8_BIT_GET(info,UI_ITEM_NAME_ALLOCATED);
	}
	W1   hasContents() const
	{
		return W8_BIT_GET(info,UI_ITEM_HAS_CONTENTS);
	}
	ICON getIcon() const
	{
		return (ICON)GET_BITS(info,UI_ITEM_ICON_ST,UI_ITEM_ICON_END);
	}
	W0   setNameAllocated(W1 true_)
	{
		info = W8_BIT_SET(info,UI_ITEM_NAME_ALLOCATED,true_);
	}
	W0   setContents(W1 true_)
	{
		info = W8_BIT_SET(info,UI_ITEM_HAS_CONTENTS,true_);
	}
	W0   setIcon(W8 icon_)
	{
		info = SET_BITS(info, UI_ITEM_ICON_ST, UI_ITEM_ICON_END, (W8)icon_);
	}
	
	UI_ITEM()
	{
		value  = 0;
		setNameAllocated(false);
	}
	
	W0   init(const SW8* name_, W1 copyName_=false, W32 itemData32_=0, W1 hasContents_=0, SW32 icon_=(ICON)0xFF)
	{
		info = 0;
		setNameAllocated(copyName_);
		setIcon(icon_);
		setContents(hasContents_);
		itemData32 = itemData32_;
		
		if (copyName_)
		{
			const W16 allocSz = strlen(name_)+1;
			name = new SW8[allocSz];
			memcpy(name, name_, allocSz);
		}
		else
			name = (SW8*)name_; 
	}
	W0   init(const SW8* name_, W32 itemData32_=0, UI_ITEM_VALUE* value_=0)
	{
		init(name_, false, itemData32_);
		value = value_; 
	}
};
	
// *** CacheItemStruct é passado à source para perguntar por mais items; 
//     a Source chama nextItem para preencher o item
class UI_ITEM_CACHE
{
public:
	W0* sourceData;
	W16 startIdx;
	W16 currIdx;
	W16 endIdx;
	UI_ITEM* items;
	W1 setItemAndAdvanceCache(UI_ITEM** item_)
	{
		if (currIdx>endIdx) 
		{
			*item_ = 0;
			return 1; // parar
		}
		else if (currIdx>=startIdx) 
		{
			*item_ = items + currIdx-startIdx;
			currIdx++;
			return 0; // preencher item e continuar
		}
		*item_ = 0;
		currIdx++;
		return 0; // ignorar continuar
	}
};

//---SOURCE TEST----------------------------------------------------------------

enum SOURCE_ACTION_RES { 	SOURCE_ACTION_RES_DONTCARE, 
							SOURCE_ACTION_RES_HIT_SAME_POINT, 
							SOURCE_ACTION_RES_NEEDS_RESTART,
							SOURCE_ACTION_RES_HIT_LEAF
						};  
enum SOURCE_ACTION     { 	SOURCE_ACTION_ENTER, SOURCE_ACTION_ESCAPE };  
class Source
{
public:
	virtual W0 init() = 0;
	virtual W16 getItems(UI_ITEM_CACHE* strct)=0;
	virtual W16 getItemsTotal()=0;
	virtual SOURCE_ACTION_RES action(UI_ITEM* item_, SOURCE_ACTION action_)=0;
	virtual W0 getTitle(SW8** title_, W1& isAllocated_) = 0;
};

#ifdef APP_USES_SD
typedef W0 (*FS_Source_FX)();
class FS_Source: public Source
{
public:
	enum FS_Source_Page { FS_Source_MBRPage, FS_Source_DirPage };
	FS_Source_Page  page; // página 0 partições, pagina = 1 dir

	FS_Source_FX fxBeforeSourceAccess;
	FS_Source_FX fxAfterSourceAccess;
	struct Dir
	{	
		std::string name;
		W32 cluster;
		Dir(SW8* name_, W32 cluster_)
		{
			name = std::string(name_);
			cluster = cluster_;
		}
	};

	static W8 default_iconAttribFunction_(FatEntry* e_)
	{
		/*
		if (e_->isUpperDir()) return ICON_DIR_BACK;
		if (e_->isDir()) return ICON_FOLDER;
		e_->ext[0]=tolower(e_->ext[0]);
		e_->ext[1]=tolower(e_->ext[1]);
		e_->ext[2]=tolower(e_->ext[2]);
		return ICON_FILE;	 */
		return ICON_NONE;
	}

	W0 purgeDirStack_()
	{
		for (W32 i=0; i< dirStack.size(); i++) 	delete dirStack[i];		
		dirStack.resize(0);
	}
	W0 init()
	{
	}
	W0 initAsMBR(FSNode* node_)
	{
		purgeDirStack_();
		node = node_;
		currDirName = std::string(ui_strs_mbr_volumes);
		page = FS_Source_MBRPage;
	}
	W0 initAsFSNode(FSNode* node_)
	{
		purgeDirStack_();
		node = node_;
		currDirName = std::string("/");
		page = FS_Source_DirPage;
	}
	
	FSNode  *node;
	std::vector<Dir*> dirStack;
	std::string currDirName;
	typedef W8 (*ATTRIBICON_FX)(FatEntry* e_);
	ATTRIBICON_FX iconAttribFunction;

	struct FS_SourcePack
	{
		FS_Source* instance;
		UI_ITEM_CACHE* ch;
	};

	FS_Source(ATTRIBICON_FX iconAttribFunction_=default_iconAttribFunction_)
	{
		fxBeforeSourceAccess = (FS_Source_FX)0;
		fxAfterSourceAccess =  (FS_Source_FX)0;
		iconAttribFunction = iconAttribFunction_;
	}
	W0 enterDir(UI_ITEM* item_)
	{
		Dir *d = new Dir((SW8*)currDirName.data(), node->cluster);
		if (!d) fatal(UI, UI_ERROR_ALLOC);
		dirStack.push_back(d);
		//node->setValid(); //<!>
		node->setAsDir(true);
		node->cluster		 = item_->itemData32;
		if (fxBeforeSourceAccess) fxBeforeSourceAccess();
		node->dir.fileCount  = fs_countDirFiles_(node->cluster);
		if (fxAfterSourceAccess) fxAfterSourceAccess();
		currDirName = std::string(item_->name);
	}
	W1 exitDir()
	{
		if (!dirStack.size()) return 1;
		Dir* dir = *(dirStack.end()-1);
		currDirName = dir->name;
		node->cluster = dir->cluster;
		if (fxBeforeSourceAccess) fxBeforeSourceAccess();
			node->dir.fileCount = fs_countDirFiles_(node->cluster);
		if (fxAfterSourceAccess) fxAfterSourceAccess();
		delete dir;
		dirStack.pop_back();
		return 0;
	}
	W0 getTitle(SW8** title_, W1& isAllocated_)
	{
		if (page == FS_Source_MBRPage)
		{
			*title_ = (SW8*)ui_strs_mbr_volumes;
			isAllocated_ = 1;
		}
		else
		{
			*title_ = (SW8*)currDirName.data();
			isAllocated_ = 1;
		}
	}

	std::string get_current_full_path()
	{
		std::string res = "";
		for (W32 i=0; i< dirStack.size(); i++)
		{
			res+=dirStack[i]->name ;
		}
		if (currDirName!="")
			res+=currDirName;
		return res;
	}

	static FS_FLAGS  fs_source_itfx(FatEntry* e_, W32 lba_, W16 off_,  W0* arg_)
	{
		if (e_->isValid())
		{
			FS_SourcePack* fsp = (FS_SourcePack*)arg_;
			UI_ITEM_CACHE* ch = fsp->ch;

			SW8 name[13];
			e_->getShortName(name);
			//printf("\n ** ui_fs_entry:  %s %i/%i/%i", name, ch->startIdx, ch->currIdx, ch->endIdx);

			UI_ITEM* nextItem;
			const W1 stopIteration = ch->setItemAndAdvanceCache(&nextItem);
			if (nextItem) 
			{
				nextItem->init(name, true,  
								(e_->clusterHi<<16)|(e_->clusterLow), 
								 e_->isDir() || e_->isUpperDir(), 
								 fsp->instance->iconAttribFunction(e_));
			
				//printf("\n ** ui_fs_entry:  %s isDir?%i", name, e_->isDir() || e_->isUpperDir());

			}
			return stopIteration? FS_ITERATE_STOP: FS_ITERATE_CONTINUE;	
		}
		return FS_ITERATE_CONTINUE;	
	}
	W16 getItems(UI_ITEM_CACHE* strct)
	{

		if (page==FS_Source_DirPage)
		{
			FS_SourcePack pack;
			pack.ch = strct;
			pack.instance = this;
			//DEBUG_PRINT("\nFS_Source:GetItems");
			if (fxBeforeSourceAccess) fxBeforeSourceAccess();
			fs_iterateDir(node, fs_source_itfx, &pack);
			if (fxAfterSourceAccess) fxAfterSourceAccess();
			
			return node->dir.fileCount;
		}

		 // partições
		SW8 volumeName[12];
		FS_TYPE type;
		for (W8 i=0; i< 4; i++)
		{
			const W8 status = fs_detectVolume_(i, false, &type, volumeName);
			if (status == RES_OK)
			{
				sprintf(mcuSharedBuffer, "%i FAT%s %s", i, (type==FS_FAT16)?"16":"32", volumeName);
				strct->items[i].init(volumeName, true, i, true, ICON_FOLDER);
			}
			else
				strct->items[i].init(ui_strs_invalid, false, i, false, ICON_ERROR);
		}
		return 4;
		
	}
	W16 getItemsTotal()
	{
		if (page==FS_Source_MBRPage)  
			return 4;
		else
		{
			//printf("\nFS_SOURCE:GETUITEMSTOTAL %i", node->dir.fileCount);
			return node->dir.fileCount;
		}
	}

	SOURCE_ACTION_RES action(UI_ITEM* item_, SOURCE_ACTION action_)
	{
		switch(action_)
		{
			case SOURCE_ACTION_ENTER:
				if (strcmp(item_->name,"..")==0) goto escape;
				
				if (!item_->hasContents()) 
				{
					return SOURCE_ACTION_RES_HIT_LEAF;
				}

				if (page==FS_Source_MBRPage) 
				{ 
					currDirName = (SW8*)ui_strs_vol_root;
					page = FS_Source_DirPage;
					if (fxBeforeSourceAccess) fxBeforeSourceAccess();
					fs_getRootDirNode(node, item_->itemData32);
					if (fxAfterSourceAccess) fxAfterSourceAccess();
					//DEBUG("\naction - %u", node->dir.fileCount);
				}
				else
				{ 
					//entrar para ex. dir "DOS"
					enterDir(item_);
				}
				return SOURCE_ACTION_RES_NEEDS_RESTART;
			break;
			case SOURCE_ACTION_ESCAPE:

				escape:
				if (page == FS_Source_MBRPage)
					return SOURCE_ACTION_RES_HIT_SAME_POINT;

				if (exitDir() == 1)
					page = FS_Source_MBRPage;
				
				return SOURCE_ACTION_RES_NEEDS_RESTART;
			break;
		}
		return SOURCE_ACTION_RES_DONTCARE;
	}
};
#endif


//-----------------------------------------------------------------------------
// UI_PAGE é uma classe abstrata que representa uma página
// de items seleccionaveis
enum UI_ANIMATE_PAGE_DIR { UI_ANIMATE_PAGE_DIR_RETURN = +1, 
						   UI_ANIMATE_PAGE_DIR_ENTER  = -1};
 
class UI_PAGE
{
public:
	UI_PAGE *returnPage;
	SW8     *title;
	W1      isTitleAllocated;
	W16     numItems;
	SW16    offset;			// selecionado absoluto
	SW16    offsetInCache;	    // selecionado mas na janela da cache
	W8      offsetInVisor;	    // selecionado no visor

	UI_ITEM   *cachedItems;
	SW16      numCachedItems;
	SW16      cacheStart;	// onde comeca a cache
	
	W1		enterButtonIsRight;
	W1		escButtonIsLeft;
	
	W0 (*hitItemFunc)(UI_ITEM* item_);
	
	W0 setTitle(SW8* title_, W1 titleIsAllocated_)
	{
		if (isTitleAllocated) delete[] title;
		if (!titleIsAllocated_)	
		{
			title = new SW8[strlen(title_+1)];
			strcpy(title, title_);
			isTitleAllocated = true;
			return;
		}
		title = title_;
		isTitleAllocated = false;
	}
	W0 reset()
	{
		numItems=0;
		cacheStart=0;
		numCachedItems=0;
		cacheStart=0;
		offset=0;			
		offsetInCache=0;
		offsetInVisor=0;
	}

	UI_PAGE()
	{
		enterButtonIsRight = true;
		escButtonIsLeft = true;
		returnPage=0;
		title=0;
		reset();
	}
	inline  W0  drawTitle_()
	{
		//printf("\n1)ui:UI_PAGE:drawTitle_() items:%i",numItems );
		#ifdef USE_VISUAL_STUDIO
			using namespace vl;
		#endif

		const Ret uiRect = Ret(0, 0, DISP_WIDTH, UI_MENU_TITLE_DY);
		disp_rep_function = disp_txRepColorBlack;
		draw_rect(uiRect, Color_Black);	///<--- <!!!>

		disp_rep_function = disp_txRepColorWhite;
		draw_text(uiRect.x + UI_MENU_TITLE_PENX, uiRect.y + UI_MENU_TITLE_PENY, title, Color_White);
		
		
		//printf("\n2)ui:UI_PAGE:drawTitle_() items:%i",numItems );

		if (numItems==0)
			sprintf(mcuSharedBuffer, "No items");
		else
			sprintf(mcuSharedBuffer, "%i/%i", ((W16)offset)+1, numItems);

		const Sz counterSz = draw_getTextSize(mcuSharedBuffer);
		draw_text(uiRect.x + DISP_WIDTH - counterSz.width- 2, 
				  uiRect.y + UI_MENU_TITLE_PENY, 
				  mcuSharedBuffer, Color_White);
	}
	virtual W0  drawLine_(const SW16 abs_offset_, const Ret r_, const W1 isSelected_){}
	virtual W0  drawItem_(const UI_ITEM* item_, const Ret r_, const W1 isSelected_)
	{
		#ifdef UI_DEBUG_ITEMS
			printf("\ni:");
		#endif
		
		W16 textcolor;
		
		if (isSelected_)
		{
			disp_rep_function = disp_txRepColorWhite;
			textcolor = Color_White;
			
			drawTextOutline = 2;
			drawTextShadowX = +3;
			drawTextShadowY = +3;
			drawTextOutlineColor = DISP_COLOR_MK_W16(0,0,0);
			drawTextShadowColor = DISP_COLOR_MK_W16(0,0,13);

		}else
		{
			disp_rep_function = disp_txRepColorBlack;
			textcolor = Color_Black;
		}
	
		if (item_->value)
		{
			if (item_->value->type == UI_ITEM_VALUE_UIPAGE)
			{
				SW8 nextStr[] = ">";
				const SW16 valueBufPixels = draw_getTextSize(nextStr).width;
				draw_text(DISP_WIDTH - valueBufPixels - UI_MENU_TEXT_RBORDER_DX,   r_.y + UI_MENU_TEXT_PENY, nextStr, textcolor);
			}
			else if (item_->value->type < UI_ITEM_VALUE_CALLBACK)
			{
				SW8 *valueBuf = new SW8[36];
				SW8* vBuf = valueBuf ;
			
				if (item_->value->prefixStr)
				{
					const W8 len = strlen(item_->value->prefixStr);
					memcpy(vBuf, item_->value->prefixStr, len); 
					vBuf+=len;
				}
				item_->value->getValueStr(vBuf);
			
				#ifdef UI_DEBUG_ITEMS
					printf(" Val:%s", vBuf);
				#endif

				if (item_->value->sufixStr)
				{
					const W8 len1 = strlen(vBuf);
					const W8 len = strlen(item_->value->sufixStr);
					memcpy(vBuf+len1, item_->value->sufixStr, len+1); 
				}
			
				const SW16 valueBufPixels = draw_getTextSize(valueBuf).width;
				draw_text(DISP_WIDTH - valueBufPixels - UI_MENU_TEXT_RBORDER_DX,   r_.y + UI_MENU_TEXT_PENY,   valueBuf, textcolor);
				delete[] valueBuf ;
			}
		}
		#ifdef APP_USES_ICONS
			if (item_->getIcon()==ICON_NONE)
			{
				draw_text(r_.x + UI_MENU_ICON_PENX,   r_.y + UI_MENU_TEXT_PENY,   item_->name, textcolor);
			}
			else
			{
				const SW16  rxDisp = (item_->getIcon()==ICON_DEFAULT)? UI_MENU_ICON_PENX: UI_MENU_TEXT_PENX;
				draw_text(r_.x + rxDisp,   r_.y + UI_MENU_TEXT_PENY,   item_->name, textcolor);
				draw_icon<1>(r_.x+UI_MENU_ICON_PENX, r_.y + UI_MENU_ICON_PENY, item_->getIcon());
			}
		#else
		
			#ifdef UI_DEBUG_ITEMS
				printf("%s", item_->name);
			#endif
			draw_text(r_.x + UI_MENU_ICON_PENX,   r_.y + UI_MENU_TEXT_PENY,   item_->name, textcolor);
		#endif

		if (isSelected_)
		{
			drawTextOutline = 0;
			drawTextShadowX = 0;
			drawTextShadowY = 0; 
		}
	}

	Ret getItemRect(W32 onScreenItemId_)
	{
		Ret uiRect = Ret(0, UI_MENU_TITLE_DY, DISP_WIDTH, UI_MENU_OPTION_DY);
		uiRect.y+=UI_MENU_OPTION_DY * onScreenItemId_;
		return uiRect;
	}

	W0          draw()
	{
	//	printf("\nui:UI_PAGE:draw() items:%i",numItems );

		#ifdef USE_VISUAL_STUDIO
			using namespace vl;
		#endif
		drawTitle_();
		Ret uiRect = Ret(0, UI_MENU_TITLE_DY, DISP_WIDTH, UI_MENU_OPTION_DY);
		
		#ifdef UI_DEBUG_ITEMS
			printf("*");
		#endif

		SW16  offsetAbsi   = offset - offsetInVisor; 
		for (W8 i=0; i< UI_MENU_SIZE; i++, uiRect.y+=UI_MENU_OPTION_DY, offsetAbsi++)
		{
			if ((offsetAbsi>=numItems) || (i!=offsetInVisor)) 
			{
				disp_rep_function = disp_txRepColorWhite;
				draw_rect(uiRect, Color_White);
				continue;
			}
			disp_rep_function = disp_txRepColor;
			draw_rect(uiRect, Color_Blue);
		}
		
		// escrever texto
		offsetAbsi -= UI_MENU_SIZE;
		uiRect.y -= UI_MENU_OPTION_DY*UI_MENU_SIZE;
		for (W8 i=0; i< UI_MENU_SIZE; i++, uiRect.y+=UI_MENU_OPTION_DY, offsetAbsi++)
		{
			const W1 selected = (i==offsetInVisor);
			if ((offsetAbsi>=numItems)) continue;

			const W16 currOffsetCache = offsetInCache - offsetInVisor + i;
			const UI_ITEM *item = cachedItems + currOffsetCache;
			drawLine_(offsetAbsi, uiRect, selected);
			#ifdef UI_DEBUG_ITEMS
				printf("-%i",i);
			#endif
			drawItem_(item, uiRect, selected);

			#ifdef UI_DEBUG_LISTSCROLL
				dispCon->setXY(0,9+i);
				dispCon->prn("offset:%i [cache off:%i]" ,offseti,  currOffsetCache);
			#endif
		}
		
		#ifdef UI_DEBUG_LISTSCROLL
			dispCon->setXY(0,8);
			dispCon->prn("[cache st:%i end:%i]", 
						cacheStart, (cacheStart+numCachedItems-1));
		#endif
	}
	UI_ITEM*    getSelectedItem()
	{
		return cachedItems+offsetInCache;
	}
	W16         getSelectedItemNumber()
	{
		return offset;
	}
	W0		    animateSelection()
	{
		#if (UI_ANIMATE_SELECTION!=0)
		UI_ITEM* item =  cachedItems + offsetInCache;
		Ret uiRect = Ret(0, 
						 UI_MENU_TITLE_DY + UI_MENU_OPTION_DY*offsetInVisor, 
						 DISP_WIDTH, 
						 UI_MENU_OPTION_DY);

		for (W8 i=0; i<UI_ANIMATE_SELECTION ; i++)
		{
			const W1 selected = i&1;
			if (selected)
			{
				disp_rep_function = disp_txRepColor;
				draw_rect(uiRect,Color_Blue);
			}
			else
			{
				disp_rep_function = disp_txRepColorWhite;
				draw_rect(uiRect, Color_Blueish);
			}
			drawLine_(offset, uiRect, selected);
			drawItem_(item, uiRect, true);
			#ifndef USE_VISUAL_STUDIO
				DELAY_MS(1)
				DRAW_UPDATE_RRECT(uiRect)
			#endif
		}
		#endif

	}
	 
	virtual W0  actionEsc() 
	{ 
		if (returnPage) uiCurrPage = returnPage;
	}
	virtual W0  actionEnter() 
	{  
		UI_ITEM *item =	getSelectedItem();
		if ((!item) || (!item->value)) return;
		if ((item->value->type == UI_ITEM_VALUE_UIPAGE) && (item->value->page.page))
		{
			returnPage = uiCurrPage;
			uiCurrPage = item->value->page.page;
			item->value->page.page->returnPage = this;
		}
		else if ((item->value->type == UI_ITEM_VALUE_CALLBACK) && (item->value->callback.call))
		{
			//printf("call");
			item->value->callback.call(item->value->callback.args);
		}
	}
	virtual W0  actionDown() {}
	virtual W0  actionUp() {}
	virtual W0  actionLeft() 
	{
		if (escButtonIsLeft) actionEsc();
	}
	virtual W0  actionRight() 
	{
		if (enterButtonIsRight) actionEnter();
	}
	virtual W0  actionRotateUp() 
	{
		UI_ITEM *item =	getSelectedItem();
		if (!item || !item->value) return;
		if (item->value->type < UI_ITEM_VALUE_CALLBACK)
			item->value->inc(+1);
	}
	virtual W0  actionRotateDown() 
	{
		UI_ITEM *item =	getSelectedItem();
		if (!item || !item->value) return;
		if (item->value->type < UI_ITEM_VALUE_CALLBACK)
			item->value->inc(-1);
	}
	virtual W0  action(UI_BUTTONS uiButton_)
	{
		switch(uiButton_)
		{
			case UI_BUTTON_LEFT:
				actionLeft();
				return;
			case UI_BUTTON_RIGHT:
				actionRight();
				return;
			case UI_BUTTON_ROTATE_UP:
				actionRotateUp();
				return;
			case UI_BUTTON_ROTATE_DOWN:
				actionRotateDown();
				return;
			case UI_BUTTON_ENTER:
				actionEnter();
				return;
			case UI_BUTTON_ESC:
				actionEsc();
				return;
			case UI_BUTTON_DOWN:
				if (offset==numItems-1) break;
				offset++;

				if (offsetInVisor==(UI_MENU_SIZE-1)) // bateu embaixo
				{
					// se der para puxar a cache para baixo
					if ( offset <= (cacheStart+numCachedItems-1) ) 
					{
					//  					    proximo
					//	                        |XXXXXX|
					//               |--numCachedItems-|
					//             cache     offset   numItems-1
					//             start      |        |
					//               |        V        |
					//               V     |012|       V
					// 0             |-----------------|
					// |-------------------------------|
					//                        ^
					//                        | offsetInCache
					//                |-------.
						cacheStart--; //empurrar a cache para trás
						offsetInCache++; 
					}
					else
					{
					//  					    proximo
					//	                        |XXXXXX|
					//               |--numCachedItems-|
					//             cache     offset   numItems-1
					//             start      |        |
					//               |        V        |
					//               V     |012|       V
					// 0             |-----------------|
					// |-------------------------------|
					//                        ^
					//                        | offsetInCache
					//                |-------.

						// senao recarregar cache
						SW16 st  = offset - (UI_MENU_SIZE-1);
						if (st<0) st = 0;
						SW16 end = st + UI_CACHEFILES_MAXSIZE-1;
						if (end > numItems) end = numItems-1;
						if ((end-st+1)<UI_CACHEFILES_MAXSIZE) 
							st = end - UI_CACHEFILES_MAXSIZE + 1;
						if (st<0) st = 0;
						cacheStart = st;
						offsetInCache = offset - cacheStart;
						numCachedItems = sourceGet_(cacheStart, end);
					}
				}
				else
				{
					offsetInVisor++;
					offsetInCache++;
				}
				actionDown();
				break;
			case UI_BUTTON_UP:
				if (offset==0) break;
				offset--;
				if (offsetInVisor==0) // bateu encima 
				{
					// se der para puxar a cache para baixo
					if ( offset > cacheStart ) 
					{
						cacheStart++;
						offsetInCache--;
					}
					else
					{
						// senao recarregar cache
						SW16 st  = offset - (UI_CACHEFILES_MAXSIZE-UI_MENU_SIZE)+1 ;
						if (st<0) st = 0;
						SW16 end = st + UI_CACHEFILES_MAXSIZE - 1;
						if (end > numItems) end = numItems-1;
						cacheStart = st;
						offsetInCache = offset - cacheStart;
						numCachedItems = sourceGet_(cacheStart, end);

					}
				}
				else
				{
					offsetInVisor--;
					offsetInCache--;
				}
				actionUp();
				break;
		}
		uiButton = UI_BUTTON_NONE;
	} 
	virtual W16 sourceGet_(W16 stIdx_, W16 endIdx_)=0;
	
};

//-----------------------------------------------------------------------------
// UI_SIMPLE_PAGE é derivada de UI_PAGE e permite inserir items
//
// UI_PAGE_SOURCE_LIST é derivada de UI_PAGE e permite associar uma
// fonte como uma base de dados, ficheiros (FS_SOURCE)
//
class UI_SIMPLE_PAGE: public UI_PAGE
{
public:
	UI_SIMPLE_PAGE()
	{
		returnPage=0;
	}
	W0   init( const SW8* title_, W16 numItems_, UI_PAGE *returnPage_=0)
	{
		title = (SW8*)title_;
		returnPage = returnPage_;
		isTitleAllocated = 0;
		numCachedItems = numItems_;
		numItems = 0;
		cachedItems = new UI_ITEM[numCachedItems];
		if (!cachedItems) fatal(UI,UI_ERROR_ALLOC);
	}

	W0  check_addItem()
	{
		if (numItems==numCachedItems)
			fatal(UI,101);
	}
	
	W0   addItem(const SW8* name_, UI_PAGE* page_)
	{
		check_addItem();
		UI_ITEM_VALUE *value = new UI_ITEM_VALUE(UI_ITEM_VALUE_UIPAGE,0);
			value->page.page  = page_;
		cachedItems[numItems].init(name_, false);
		cachedItems[numItems].value = value;
		numItems++;
	}
	W0   addItem(const SW8* name_,  UI_CALLBACK callback_, W0* args_=0)
	{
		check_addItem();
		UI_ITEM_VALUE *value = new UI_ITEM_VALUE((SW8*)0);
			value->type  = UI_ITEM_VALUE_CALLBACK;
			value->callback.call  = callback_;
			value->callback.args = args_;
		cachedItems[numItems].init(name_, false);
		cachedItems[numItems].value = value;
		numItems++;
	
	}
	W0   addItem( const SW8* name_, UI_ITEM_VALUE* value_=0)
	{
		check_addItem();
		cachedItems[numItems].init(name_, 0, value_);
		numItems++;
	}
	W0   addItem( const SW8* name_, W32 itemData32_=0, UI_ITEM_VALUE* value_=0)
	{
		check_addItem();
		cachedItems[numItems].init(name_, itemData32_, value_);
		numItems++;
	}
	W16 sourceGet_(W16 stIdx_, W16 endIdx_){ return numItems; }

};


typedef W0(*UI_PAGE_SOURCE_LIST_FX)(UI_ITEM*);
class UI_PAGE_SOURCE_LIST: public UI_PAGE
{
public:
	Source* source;
 
	UI_PAGE_SOURCE_LIST_FX leaf_function;

	UI_PAGE_SOURCE_LIST()
	{
		leaf_function = (UI_PAGE_SOURCE_LIST_FX)0;
		numCachedItems = 0;
		source=0;
		numItems = 0;
		title = 0;
		cachedItems = new UI_ITEM[UI_CACHEFILES_MAXSIZE];
		if (!cachedItems) fatal(UI, UI_ERROR_ALLOC);
	}
		
	W0  actionEnter()
	{
		returnPage = uiCurrPage;
		
		//printf("\n*UI_PAGE_SOURCE_LIST:ENTER");
		animateSelection(); 
		
		UI_ITEM* item = getSelectedItem();
		SOURCE_ACTION_RES res = source->action(item, SOURCE_ACTION_ENTER);
		
		if (res == SOURCE_ACTION_RES_NEEDS_RESTART) 
		{
			init(source,leaf_function);
		}
		if (res == SOURCE_ACTION_RES_HIT_LEAF)
		{
			if (leaf_function)
			leaf_function(item);
			return;
		}

		SW8 *newTitle=0;
		W1   allocNewTitle = false;
		source->getTitle(&newTitle, allocNewTitle); 
		setTitle(newTitle, allocNewTitle);
		
		animatePage(UI_ANIMATE_PAGE_DIR_ENTER); 
	}
	W0  actionEsc()
	{
		// para se notar que se premiu o botão
		#ifdef DRAW_USES_BUFFER
			draw_addColor(dispBuf,1,0,(DISP_WIDTH*DISP_HEIGHT));
			DRAW_UPDATE
		#endif

		SOURCE_ACTION_RES res = source->action(cachedItems+offsetInCache, SOURCE_ACTION_ESCAPE);

		if (res == SOURCE_ACTION_RES_HIT_SAME_POINT) 
			uiCurrPage = returnPage;
		else if (res == SOURCE_ACTION_RES_NEEDS_RESTART) 
			init(source);
		
		SW8* newTitle;
		W1 isAllocated;
		source->getTitle(&newTitle, isAllocated); 
		
		setTitle(newTitle, isAllocated);

		animatePage(UI_ANIMATE_PAGE_DIR_RETURN); 
	}
	W16 sourceGet_(W16 stIdx_, W16 endIdx_)
	{
		//DEBUG_PRINT("\nui:Source_List: sourceGet_");
		
		for (W8 i=0; i< UI_CACHEFILES_MAXSIZE; i++)
		{
			if (cachedItems[i].isNameAllocated()) delete[] cachedItems[i].name;
			cachedItems[i].setNameAllocated(false);
			cachedItems[i].name = 0;
		}
		if (source->getItemsTotal()==0) return 0;

		UI_ITEM_CACHE strct;
		strct.items     = cachedItems;
		strct.startIdx  = stIdx_;
		strct.currIdx   = 0;
		strct.endIdx    = endIdx_;
		source->getItems(&strct);
		
		return endIdx_-stIdx_;
	}
	W0  init(Source* source_, UI_PAGE_SOURCE_LIST_FX leafe_fx_ = (UI_PAGE_SOURCE_LIST_FX) 0)
	{ 
		leaf_function = leafe_fx_ ;
		reset();
		if (!source_) return;
		source = source_;

		source->init();
		numItems = source->getItemsTotal();

		//printf("\nui:UI_PAGE_SOURCE_LIST:init(Source) items:%i",numItems );
		numCachedItems = sourceGet_(0, VL_MIN(UI_CACHEFILES_MAXSIZE-1, numItems - 1));
		SW8* newTitle;
		W1 isAllocated;

		source->getTitle(&newTitle, isAllocated); 

		setTitle(newTitle, isAllocated);

	}
};

//-----------------------------------------------------------------------------
// UI_SIMPLE_PAGE é derivada de UI_PAGE e permite visualizar um
// ficheiro de texto
#ifdef APP_FILETEXT_ON
class UI_PAGE_TEXT: public UI_PAGE
{
	FSNode  *node;
	W16 totalLines;
	W16 curPosX, maxCols;
	W16 bufferOffset;
	W32 iterateLinesFromBegin_(W16 stopAt=-1)
	{
		fs_seek(node, 0, 0);
		W16 lineSize = 0;
		W16 totalLines_ = 0;
		W32 offset_ = 0;
		while(!fs_isEOF(node))
		{
			W8 bytesCount = fs_read(node, 512, 0);
			for (W16 i=0; i< bytesCount; i++, offset_+=bytesCount, lineSize+=bytesCount)
			{
				if (fs_buff[i]=='\n') 
				{
					lineSize+=i;
					offset_+=i;
					maxCols   = VL_MAX(lineSize, maxCols);
					lineSize=0;
					if (stopAt!=-1 && totalLines_==stopAt) return offset_;
					totalLines_++;
				}
			}
		}
		return totalLines_;
	}
	W0 drawLine_(const SW16 line_, const Ret r_, const W1 isSelected_)
	{
		if (line_==0) 
			bufferOffset = iterateLinesFromBegin_(line_);

		W16 curPosXrel = curPosX;
		//bufferOffset+curPosX pode cruzar buffer
		while(bufferOffset + curPosXrel>=512)
		{
			curPosXrel-=512-bufferOffset;
			fs_read(node, 512, 0);
			bufferOffset=0;
		}
		// procurar se a linha acaba antes do ecra 
		W8 i;
		W1 foundNext = false;
		for (i=0; i< DISP_WIDTH; i++)
			if (fs_buff[i]=='\n') { foundNext=true; goto skipText;}
	skipText:
		draw_text(r_.x, r_.y + fontBlockY, (SW8*)fs_buff+curPosXrel, Color_Black, FONT_IS_MONOSPACED, i);
		if (foundNext) return;
		// senao encontrou a proxima procurar
		while(!fs_isEOF(node))
		{
			for (; i< 512; i++)
				if (fs_buff[i]=='\n') { return; }
			fs_read(node, 512, 0);
			i = 0;
		}
	}
	W0 actionLeft()
	{
		if (curPosX==0) return;
		curPosX--;
	}
	W0 actionRight()
	{
		if (maxCols<DISP_WIDTH) return;
		if (curPosX==maxCols-DISP_WIDTH) return;
		curPosX++;
	}
	W0 actionUp()
	{
		if (offset==0) return;
		offset--;
	}
	W0 actionDown()
	{
		if (totalLines < UI_MENU_SIZE) return;
		if (offset==totalLines-UI_MENU_SIZE-1) return;
		offset++;
	}
	W0 actionEsc()
	{
	}
	UI_PAGE_TEXT(FSNode* node_)
	{
		bufferOffset = 0;
		curPosX = maxCols = 0;
		node = node_;
		totalLines = iterateLinesFromBegin_();
	}
};


#endif


// faz a transição entre páginas
W0 animatePage(SW8 dir_)
{
	#ifdef DRAW_USES_BUFFER
	#if (UI_ANIMATE_DIR!=0)
		#if (UI_ANIMATE_DIR_BUFF==1)
			W16 *tmp= dispBuf, tmp2,
				*newBuff = new W16[DISP_WIDTH*DISP_HEIGHT];
			if (!newBuff) fatal(UI,UI_ERROR_ALLOC);
			memset(newBuff, 0xFF, DISP_WIDTH*DISP_HEIGHT*2);
			dispBuf = newBuff;
			uiCurrPage->draw();	
			dispBuf = tmp;
			//for (W8 i=1; i< 2; i++)
			//{
			//	draw_addColor(dispBuf,i*2,0,(DISP_WIDTH*DISP_HEIGHT));
			//	DRAW_UPDATE
			//}
		#endif
				
		Ret uiRect = Ret(0, UI_MENU_TITLE_DY, DISP_WIDTH, DISP_HEIGHT-UI_MENU_TITLE_DY);
		const W8 x1 =  uiRect.x + uiRect.lx;
		const W8 y1 =  uiRect.y + uiRect.ly;
		if (dir_==UI_ANIMATE_PAGE_DIR_ENTER) // left
		{
			for (W8 i=0; i< DISP_WIDTH-UI_ANIMATE_DIR; i+=UI_ANIMATE_DIR)
			{
				#if (UI_ANIMATE_DIR_BUFF==0)
					draw_roi_x = DISP_WIDTH - i - UI_ANIMATE_DIR;
					draw_moveBuffer(-UI_ANIMATE_DIR);
					uiCurrPage->draw();
				#else
					draw_moveBufferAndCopy(-UI_ANIMATE_DIR, i, newBuff);
				#endif
				DRAW_UPDATE
				//DRAW_UPDATE_RECT(uiRect.x, uiRect.y, x1, y1)
			}
		}
		else if (dir_==UI_ANIMATE_PAGE_DIR_RETURN)
		{
			for (W8 i=0; i< DISP_WIDTH-UI_ANIMATE_DIR; i+=UI_ANIMATE_DIR)
			{
				#if (UI_ANIMATE_DIR_BUFF==0)
					draw_roi_x = -DISP_WIDTH + i + UI_ANIMATE_DIR;
					draw_moveBuffer(+UI_ANIMATE_DIR);
					uiCurrPage->draw();
				#else
					draw_moveBufferAndCopy(+UI_ANIMATE_DIR, i, newBuff);
				#endif
				DRAW_UPDATE
				//DRAW_UPDATE_RECT(uiRect.x, uiRect.y, x1, y1)
			}
		}

		#if (UI_ANIMATE_PAGE_BUFF==1)
			memcpy(dispBuf, newBuff, DISP_WIDTH*DISP_HEIGHT*2);
			delete[] newBuff;
		#else
			draw_roi_x=0;
		#endif
		DRAW_UPDATE
	#endif
	#endif
}

