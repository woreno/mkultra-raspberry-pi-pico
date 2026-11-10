
enum	FS_TYPE { FS_FAT_NO=0, FS_FAT12=1, FS_FAT16=2, FS_FAT32=4 };


VL_PACK_BEGIN
struct VL_PACK_MIDDLE BootHeader 
{
public:
	//
	W8   startJumpCode[3];				     //+0
	W8   nameSYSeOEM[8];				     //+3
	W16  numBytesPerSector; 			     //+11
	W8   numSectorsPerCluster;			     //+13
	W16  numSectorsReserved;			     //+14
	W8   numFATs;							 //+16
	W16  numEntriesAtRootDir,				 //+17
		 numLogicalSectors;					 //+19
	W8   mediaType;							 //+21
	W16  numSectorsPerFAT,				     //+22
		 numSectorPerTrack,				   	 //+24
		 numHeads;							 //+26
	W32									 	  //
			numHiddenSectors,			     //+28
			numHiddenSectorsIfBigger;	     //+32
		
	//extended bios parameter block
	union
	{
		VL_PACK_BEGIN
		struct VL_PACK_MIDDLE DOS4_
		{
			W8   driveNumber, //+36
					reserved1,  //+37
					signature;  //+38
			W32  volumeSerial;  //+39
			W8   volumeLabel[11]; //+43
			SW8   systemID[8]; //+54
		} dos4;
		VL_PACK_END

		VL_PACK_BEGIN
		struct VL_PACK_MIDDLE DOS7_
		{
			W32 numSectorsPerFAT;	//+36
			W16 reserved1;			//+40
			W16 versao;				//+42
			W32 numClusterRootDir;	//+44
			W16 numSectorInfoFS;	//+48
			W16 numSectorBackup;	
			W8  reserved2[12];
			W8  driveNumber;
			W8  flags,
				signature;
			W32 volumeSerial;
			W8  volumeLabel[11];
			SW8  systemID[8];
		} dos7;
		VL_PACK_END
	} ext;

	static FS_TYPE checkSign_(SW8* sign_)
	{
		//printf("\n!!!!%c%c%c%c%c", sign_[0], sign_[1], sign_[2], sign_[3], sign_[4]);

		if (strncmp(sign_,"FAT", 3)!=0)
			return FS_FAT_NO;

		if (sign_[3]=='1') 
		{
			if (sign_[4]=='2') return FS_FAT12;
			else if (sign_[4]=='6') return FS_FAT16;
			else return FS_FAT_NO;
		}
		else if (sign_[3]=='3' &&  sign_[4]=='2') return FS_FAT32;
		return FS_FAT_NO;
	}

	W0 copyVolumeName(SW8* name_)
	{
		SW8* src = 0;
		if (checkSign_(ext.dos7.systemID)!=FS_FAT_NO)
			src = (SW8*) ext.dos7.volumeLabel;
		else if (checkSign_(ext.dos4.systemID)!=FS_FAT_NO)
			src = (SW8*) ext.dos4.volumeLabel;
		
		if (src)
		{
			memcpy(name_, src, 11);
			name_[11]=0;
			SW8* n = name_+=11;
			while(n!=name_ && *n==' ') { *n=0; n--;} 
			return;
		}
		sprintf(name_, "<Unamed>");
	}

	inline FS_TYPE detectFatType()
	{

		FS_TYPE type = checkSign_(ext.dos7.systemID);
		
		if (type!=FS_FAT_NO) return type;

		type = checkSign_(ext.dos4.systemID);
		if (type!=FS_FAT_NO) return type;

		//if (numSectorsPerCluster)
		//{
		//	const W32 total_clusters = numLogicalSectors/numSectorsPerCluster;
		//	if (total_clusters < 4085) return FS_FAT12;
		//	else if(total_clusters < 65525) return FS_FAT16;
		//	else if (total_clusters < 268435445) return FS_FAT32;
		//}
		return FS_FAT_NO;
	}
} VL_PACK_END; 


VL_PACK_BEGIN
struct VL_PACK_MIDDLE FatEntry 
{
	W8 name[8];
	W8 ext[3];
	W8 attribs;
	W16 reserved;
	W16 timeCreation,
		dateCreation,
		dateLastAccess;
	W16 clusterHi;
	W16 timeModification;
	W16 dateModification;
	W16 clusterLow;
	W32 fileSize;

	inline W32 getCluster()
	{
		return (((W32)clusterHi)<<16)+ (W32)clusterLow;
	}
	inline W0 setCluster(W32 c_)
	{
		clusterLow = c_;
		clusterHi = c_>>16;
	}
	W1 isParent()
	{
		return (strncmp(".. ",(SW8*)name,3)==0);
	}
	W1 isSelf()
	{
		return (strncmp(". ",(SW8*)name,3)==0);
	}
	W0 setShortName(SW8* str_)
	{
		memset(name, ' ', 8+3);
		if (strcmp(str_,"..")==0)
		{
			name[0]='.';
			name[1]='.';
			return;
		}
		if (strcmp(str_,".")==0)
		{
			name[0]='.';
			return;
		}

		W8* dst = name;
		while(*str_)
		{
			if (*str_=='.') { dst = ext; str_++; continue;  }
			*dst = toupper(*str_);
			dst++;
			str_++;
		}
	}
	W0 getShortName(SW8* str__)
	{
		SW8* str_ = str__;
		for (W16 i=0; i< 8; i++, ++str_)
		{
			if (name[i]==0x20) break;
			*str_ = name[i];
		}
		if (ext[0]!=0x20)
		{
			*str_ = '.';
			str_++;
			for (W16 i=0; i< 3; i++, str_++)
			{
				if (ext[i]==0x20) break;
				*str_ = ext[i];
			}
		}
		*str_ = 0;
	}
	W1 isVolumeLabel()
	{
		return (attribs&0x8)!=0; 
	}
	W1 isDir()
	{
		return (attribs&0x10)!=0; 
	}
	W0 setAs(W1 isDir_)
	{
		attribs=(isDir_?0x10:0);
	}
	W1 isUpperDir()
	{
		return (name[0]='.' && name[1]=='.' && name[2]==' ');
	}
	W1 isValid()
	{
		if (name[0]=='.' && name[1]==' ') return false;
		return !(attribs==0xF||attribs&0x8||name[0]==0||name[0]==0xE5); 
	}
	W0 setFree()
	{
		name[0]=0xE5;
	}
	W1 isFree()
	{
		return ((name[0]==0)||name[0]==0xE5); 
	}
	W0 print()
	{
		SW8 fn[8+1+3+1];
		fn[0]=0;
		if (isValid()) getShortName(fn);
		FS_DEBUG_LOG("\n%12s| c: %6Xh | dir?:%i | free:%i | at:%02X | sz:%i", fn,  getCluster(), isDir(), isFree(), attribs, fileSize)
		//FS_DEBUG_LOG("\n%8s %3s | c: %Xh | dir?:%i | free:%i", name, ext, getCluster(), isDir(), isFree())
	}

} VL_PACK_END;

	
#define	MBR_HAVE_LBA	 0x4
#define MBR_START		 0x01BE
// expande a codificação CHS em 3 bytes da BIOS/MBR
#define MBR_EXTRACT_C(A) ((((W32)(A[1]>>6))<<8) | (W32)A[2] ) 
#define MBR_EXTRACT_H(A) (A[0])  
#define MBR_EXTRACT_S(A) ((W32)A[1] & 0x3F)

VL_PACK_BEGIN
struct VL_PACK_MIDDLE MBRPart 
{
	W8  status;
	W8  CHSStart[3];
	W8  type;
	W8  CHSEnd[3];
	W32 LBAStart;
	W32 LBASize;
} VL_PACK_END;

#define FAT_TABLE_SET_VALUE_RELATIVE(CLUSTER_, VALUE_)\
	if (fs_type==FS_FAT32)							  \
		((W32*)FS_FAT_BUFFER)[CLUSTER_] = VALUE_;	  \
	else											  \
		((W16*)FS_FAT_BUFFER)[CLUSTER_] = VALUE_;	  \

#define FAT_TABLE_GET_VALUE_RELATIVE(CLUSTER_, VALUE_)	\
	 VALUE_ =  (fs_type==FS_FAT32)?						\
		 (((W32*)FS_FAT_BUFFER)[CLUSTER_]):				\
		 (((W16*)FS_FAT_BUFFER)[CLUSTER_]);				\

#define FAT_TABLE_GET_FATPAGE_FOR_CLUSTER(ABS_CLUSTER_)\
		( (ABS_CLUSTER_)/ ((FS_FAT_CACHE_SZ)/(fs_type)) )

#define FAT_TABLE_GET_RELATIVE_CLUSTER(ABS_CLUSTER_, FAT_NUM_)\
			((ABS_CLUSTER_) -  (FAT_NUM_) * ((FS_FAT_CACHE_SZ)/(fs_type)))

#define FAT_TABLE_GET_NUMBER_PER_PAGE() \
			((FS_FAT_CACHE_SZ)/(fs_type))

