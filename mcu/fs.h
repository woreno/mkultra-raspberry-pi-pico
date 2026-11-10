//<!> fseek não grava (não faz append)
#define FS_DEBUG_LOG(X,...) { printf(X,__VA_ARGS__);}

//#define FS_DEBUG_WR
//#define FS_DEBUG_NEXT_WR_CLUSTER 
//#define FS_DEBUG_NEXT_CLUSTER 
//#define FS_DEBUG_DELETE 
//#define FS_DEBUG_FATCACHE
//#define FS_FSNODE_HAS_PRINT
//#define FS_DEBUG_GET_NODE_ENTRY
//#define FS_DEBUG_UPDATE_NODE_ENTRY
//#define FS_PRINTENTRY_PRINTS_ALL_CLUSTERS
//#define FS_DEBUG_FASTFORMAT
//#define FS_DEBUG_CLUSTERLINK

// snippets:
//		FSNode pathNode;
//		fs_getRootDirNode(&pathNode);
//		printf("\nrtdir:");
//		fs_iterateDir(&pathNode, fs_itfx_prnEntry, 0);


#include "fatHeads.h"
#define FS_IS_OLDER_DIRROOT(CLUSTER_IDX_) (((fs_type==FS_FAT12) || (fs_type==FS_FAT16)) && (CLUSTER_IDX_==0))

// coisas por fazer:
//   em FAT32 
//     ...printEntry e outros nao funciona para nomes grandes 
//     ...createFileNode nao funciona para nomes grandes

//  4/10/2026 adicionei a fs_detectVolume_ a partição do tipo MBR 0xE (16 mas com 13h extendida)
//            mas nao li do que se trata


//FS_OK se conseguir
//FS_FAIL se não
//definir lá fora DEV_RD_BLOCK, DEV_WR_BLOCK, DEV_TOTALHEADS, DEV_TOTALSECTPERCILS

// I. Tabela fat (segmentada) e sua cache
//	
//  <fs_currFatNum>						o segmento da FAT carregada (em cache)
//  fs_get_fat_total_segments_			retorna o nº de segmentos que segmentam a FAT, 1 cache = 1 segmento  
//  fs_get_fat_cache_details_			retorna o numero da tabela FAT para um clusterIdx_
//  fs_load_fat_cache_idx_				carregar a tabela para um clusterIdx_ (e define fs_currFatNum)
//  fs_load_fat_cache_for_				carrega a tabela do clusterIdx_ (senao carregada)
//  fs_save_fat_cache_					gravar o segmento da FAT carregada para o disco
//  <struct FS_FATIterator>				iterador da tabela FAT dado um cluster de começo


// II. Auxiliares
//  fs_countFreeClusters_				conta os clusters livres		
//  fs_getBasePathAndFilenameSize		aux. separa o <nome do ficheiro> do <caminho> dado os dois
//  fs_getClusterSizeInBytes			retorna o tamanho dum cluster (= cluster size em bytes)
//  fs_spaceClusters					conta o espaço total/livre ?!!!??????????????????????
//  fs_space							retorna o espaço total/livre ?!!!??????????????????????
//  fs_bytesToClusters					dado um tamanho em bytes devolve quantos clusters

// III. Inicialização
//  fs_detectVolume_					inicializa o FS numa particao

// IV. Clusters
//  fs_getClusterLBA_					retorna o LBA do próximo clusters
//  fs_getNextClusterIdx_				retorna o próximo cluster dado o currente
//  fs_getLastClusterIdx_				retorna o último cluster dum ficheiro (quando chega ao terminador)
//  fs_searchFreeCluster_				procura o primeiro cluster LIVRE
//  fs_setClusterPageToZero_			apaga a ZERO e grava um cluster
//  fs_setFatClusterValue_				dado um cluster, define qual é o próximo
//  fs_linkClusterRange_				dado um intervalo de clusters, liga uns-a-outros em sequência
//  fs_allocCluster_					dado um cluster, navega até ao fim, e ALOCA um novo cluster, devolve esse último
//  fs_allocNewCluster_					dado um cluster, ALOCA um novo cluster de FIM, a seguir a este, devolve esse último
//  fs_allocNewClusters_				dado um cluster, ALOCA uma sequência ligada de clusters
//  fs_getNextClusterIdxForRW_			retorna o próximo cluster, mas se a operação for de 
//										 escrita, ALOCA novos clusters no fim

// V. Funções auxiliares chamadas na iteração de diretórios (nome-a-nome)
//  fs_itfx_search_						procura 1 nome
//  fs_itfx_prnEntry					imprime 1 nome
//  fs_itfx_count_						conta 1 nome

// VI. Funções auxiliares chamadas na iteração de diretório
//  fs_iterateEntries_					itera as entradas
//  fs_iterateDir_						itera um diretório chamando a fs_iterateEntries_
//  fs_countDirFiles_					chama a iterateDir com fs_itfx_count_

// VII. API
//  <FSNode>							descritor dum ficheiro ou diretório que contêm
//										 o seu cluster, tamanho, posição de 
//										 leitura/escrita atual, etc
//
//  fs_checkNode_						(auxiliar) verifica se o nó está válido, e se
//										 estiver noutra partição da atual, reinicia
//										 o código para esta partição
//  fs_getNodeEntry						qual é o LBA+offset de entrada, no diretório parente?
//  fs_getFilenameExt					(auxiliar) dado um NOME de ficheiro devolve EXTENSAO
//  fs_isRootDirNode					dado um nó retorna se é o DIRETÓRIO RAIZ
//  fs_getRootDirNode					dada uma partição devolve o DESCRITOR para a DIRETÓRIO RAIZ
//  fs_iterateDir						chamada a fs_iterateDir_ com uma FUNCAO <generico>
//  fs_search							chamada a fs_iterateDir_+fs_itfx_search_ para procurar um nome
//
//  fs_getNodeAtAbsPath					retorna o DESCRITOR dado um caminho 
//  fs_reserveClusters_					faz uma RESERVA sequencial de clusters
//  fs_updateNodeEntry_					grava O DESCRITOR opcionalmente alterando o NOME no seu diretório parente
//  fs_createEntry_						cria uma NOVA ENTRADA num diretório
//
//  fs_rw_								(auxiliar) operação escrita/leitura/reposicionamento
//  fs_seek								reposicionamento
//  fs_read								leitura
//  fs_write							escrita
//  fs_tell								devolve o tamanho
//  fs_isEOF							devolve se está no fim
//
//  fs_createNode_						(auxiliar) cria o DESCRITOR
//  fs_createDirNode					devolve o DESCRITOR de diretório dado caminho (opcionalmente criando-o)
//  fs_createFileNode					devolve o DESCRITOR de ficheiro dado caminho (opcionalmente criando-o)
//  fs_createFileNodeExt			
//  fs_init								inicializa FS
//
//  fs_fastWriteBegin					iniciar ESCRITA SEQUENCIAL
//  fs_fastWrite						ESCRITA SEQUENCIAL
//  fs_fastWriteEnd						fechar ESCRITA SEQUENCIAL
//
//  fs_getFilename						dado um CAMINHO devolve o NOME DA ENTRADA
//
//  fs_renameFile						RENOMEA uma ENTRADA
//  fs_delete							ELIMINA uma ENTRADA
//  fs_close							grava alterações AO FICHEIRO/DIRETORIO

//  fs_fastformat						formata rápido uma entrada MBR



enum FS_FLAGS   { FS_OK = 0,
				  FS_FAIL = 1,
				  FS_NO_MBR_SIGN=2,
				  FS_NO_MBR_PART,		
				  FS_MBR_PART_INACTIVE, 
				  FS_IS_FAT12,			
				  FS_NO_VALID_FAT,		
				  FS_ROOT_DIR_EMPTY,	
				  FS_FILE_NOT_FOUND,
				  FS_FAIL_TO_UPDATE,
				  FS_ROOTDIR_NO_MORE_SPACE,
				  FS_CACHE_NOT_ASSIGNED,
				  FS_OP_READ,
				  FS_OP_WRITE,
				  FS_OP_SEEK,
				  FS_NEW_CLUSTER,
				  FS_PARENT_DIR,
				  FS_CURRENT_DIR,
				  FS_END_OF_FILE,
				  FS_NO_MORE_SPACE,
				  FS_FOUND, 
				  FS_NOT_FOUND,
				  FS_ITERATE_STOP, 
				  FS_ITERATE_CONTINUE 
				};

// mneumonicas
#define FS_DEV_GETBLK(X_,BUF_)	DEV_RD_BLOCK((X_),BUF_)
#define FS_DEV_SAVEBLK(X_,BUF_)	DEV_WR_BLOCK((X_),BUF_)
#define FS_GETBLK(X_,BUF_)		DEV_RD_BLOCK((X_)+fs_LBAStart,BUF_)
#define FS_SAVEBLK(X_,BUF_)		DEV_WR_BLOCK((X_)+fs_LBAStart,BUF_)
 // se FS_FAT_USE_CACHE_WITH_SIZE == X
//   entao deixa a ultima página de X bytes da FAT
//   em memória em fs_fatBuff; se a próxima consulta 
//   for na mesma página, não é necessário recarregar
//   outra página; invalida nas escritas
// se FS_FAT_USE_CACHE_WITH_SIZE == 0
//   entao é usado o mesmo fs_buff usado geralmente
//   e a FAT é logo descartada assim que usada

#if (FS_FAT_USE_CACHE_WITH_SIZE==0)
	#define FS_FAT_CACHE_SZ		512
	#define FS_FAT_CACHE_LBAS	1
	#define FS_FAT_BUFFER		fs_buff
#else
	#define FS_FAT_CACHE_SZ		FS_FAT_USE_CACHE_WITH_SIZE
	#define FS_FAT_CACHE_LBAS   (FS_FAT_USE_CACHE_WITH_SIZE/512)
	#define FS_FAT_BUFFER		fs_fatBuff
#endif

FS_TYPE	  fs_type;

//#if FS_USES_EXTERNAL_BUFFER 
//	W8		 *fs_buff = (W8*)FS_USES_EXTERNAL_BUFFER; 
//#else
	W8        fs_buff [512] VL_ALIGN(4);
//#endif 

#if (FS_FAT_USE_CACHE_WITH_SIZE!=0)
	W8      fs_fatBuff[FS_FAT_CACHE_SZ] VL_ALIGN(4);	  // cache da FAT
	W16     fs_currFatNum = 0xFFFF;  // qual e a parte da FAT em cache
#endif
#define FS_CLUSTERIDX_IS_TERMINATOR(X_)  ((X_) >= fs_endInvalidIdx)

// descartar o Header e manter so as variaveis importantes
// para o FS funcionar...
W8		fs_currMBR = 0xFF;
W32 
		fs_endClusterIdx,
		fs_endInvalidIdx,
		fs_LBAStart,			// onde começa o FS (absoluto)
		fs_fat_LBAStart,         // ...
		fs_fatBackup_LBAStart,   // ... todos estes relativos
		fs_fat_LBASize,          // ... a fs_LBAStart
		fs_allocs_LBAStart,      // ...
		fs_rootDir_cluster;

W8      fs_cluster_LBASize,
		fs_rootDir_LBASize;   

W0       fs_invalidate_fat_cache_()
{ 
	fs_currFatNum=0xFFFF; 
}

// esta a dar zero!!!
#define FS_NUMCLUSTERS_FOR_ROOTDIR (fs_rootDir_LBASize * fs_cluster_LBASize)

enum FS_SearchEntry { FS_SearchEntry_Name, FS_SearchEntry_Free, FS_SearchEntry_Cluster };
VL_PACK_BEGIN
struct VL_PACK_MIDDLE FS_Search 
{
	W1  found;
	W1  isDir;
	W8  searchType;
	union
	{
		struct
		{
			SW8 *nameToMatch;
		};
		struct
		{
			W32 clusterToMatch;
		};
	
	} in;
	
	struct
	{
		W32  fizeSize;
		W32  foundCluster;
		W32  entryLBA;
		W16  entryOffset;
	}res;
	
	FS_Search(FS_SearchEntry searchTy_): searchType(searchTy_)
	{
		found=false;
		isDir=false;
	}
} VL_PACK_END;
typedef FS_FLAGS(*FS_ITERATOR_FUNCTION)(FatEntry* e_, W32 lba_, W16 off_, W0* arg_);

W16 fs_get_fat_total_segments_()
{
	// retorna o nº de segmentos que segmentam a FAT 
	// 1 cache = 1 segmento
	return (fs_fat_LBASize)/FS_FAT_CACHE_LBAS;
}
// retorna o numero da tabela do clusterIdx_
W0 fs_get_fat_cache_details_(const W32 clusterIdx_, W16& table_num_, W16& relativeIdx_)
{
	const W16 clustersPerSegment = (FS_FAT_CACHE_SZ)/(fs_type);
	table_num_   = clusterIdx_/clustersPerSegment;
	relativeIdx_ = clusterIdx_%clustersPerSegment;
}
W0 fs_load_fat_cache_idx_(const W16 idx_)
{
	const W32 lbaStart = idx_* FS_FAT_CACHE_LBAS;
	W8* fat = (W8*)FS_FAT_BUFFER;
	for (W16 i=0; i< FS_FAT_CACHE_LBAS; i++, fat+=512)
	{
		FS_GETBLK((fs_fat_LBAStart + lbaStart + i), fat);
	}
	fs_currFatNum = idx_;
}
// carrega a tabela do clusterIdx_ (senao carregada)
W16 fs_load_fat_cache_for_(const W32 clusterIdx_)
{
	#ifdef FS_DEBUG_FATCACHE
		FS_DEBUG_LOG("\nLoad Fat for c:%Xh", clusterIdx_);
	#endif

	W16 relativeIdx, newFatNum;
	fs_get_fat_cache_details_(clusterIdx_, newFatNum, relativeIdx);	
	// newFatNum é a página nova na FAT
	// se usar cache, só carrega se mudou a página 
	// senão carrega sempre porque FS_FAT_BUFFER é o mesmo
	// usado para ler o conteudo dos ficheiros/diretorios
	#if (FS_FAT_USE_CACHE_WITH_SIZE!=0)
		if (newFatNum != fs_currFatNum) 
			fs_load_fat_cache_idx_(newFatNum); 
	#else
		fs_load_fat_cache_idx_(newFatNum);
	#endif
	
	fs_currFatNum = newFatNum ;

	#ifdef FS_DEBUG_FATCACHE
		FS_DEBUG_LOG("\nNew Fat Cache %i", fs_currFatNum);
	#endif

	// devolver o cluster relativo à página
	return relativeIdx;  
}
W0  fs_save_fat_cache_()
{
	#ifdef FS_DEBUG_FATCACHE
		FS_DEBUG_LOG("\nSave Fat Cache %i",fs_currFatNum);
	#endif

	if (fs_currFatNum==0xFFFF) fatal(FS, FS_CACHE_NOT_ASSIGNED);

	const W32 lbaStart = fs_currFatNum* FS_FAT_CACHE_LBAS;
	W8* fat = (W8*)FS_FAT_BUFFER;
	for (W16 i=0; i< FS_FAT_CACHE_LBAS; i++, fat+=512)
	{
		FS_SAVEBLK(fs_fat_LBAStart + lbaStart + i, FS_FAT_BUFFER);
		FS_SAVEBLK(fs_fatBackup_LBAStart + lbaStart + i, fat);
	} 
}



template<typename TYPE_, typename ACTION_STRUCT_>
struct FS_FATIterator
{
	static inline W0 run(ACTION_STRUCT_& action_, W32 beginCluster_=3)
	{
		const W16 numCaches = fs_get_fat_total_segments_();
		const W32 numEntries = (FS_FAT_CACHE_SZ/sizeof(TYPE_));

		W16 cacheIdx, relClusterIdx;
		W32 clusterIdx = beginCluster_;
		fs_get_fat_cache_details_(clusterIdx, cacheIdx, relClusterIdx);
		TYPE_* fat = ((TYPE_*)FS_FAT_BUFFER);

		for (W16 i=cacheIdx; i< numCaches; i++)			
		{
			fs_load_fat_cache_idx_(i);
			for (; relClusterIdx< numEntries; relClusterIdx++, clusterIdx++)
			{
				if (action_.run(clusterIdx, fat[relClusterIdx]))
					goto skip;
			}
			relClusterIdx = 0;
		}
		skip:
		fs_invalidate_fat_cache_();
	}
};
W32      fs_countFreeClusters_()
{
	struct FS_CountFree
	{
		W32 freeClusters;
		FS_CountFree()
		{
			freeClusters = 0;
		}
		W1 run(W32 clusterIdx_, W32 clusterValue_)
		{
			if (clusterValue_==0) freeClusters++;
			return false;
		}
	} action;

	if (fs_type==FS_FAT16)
		FS_FATIterator<W16,FS_CountFree>::run(action);
	else
		FS_FATIterator<W32,FS_CountFree>::run(action);

	return action.freeClusters;
}

W0       fs_getBasePathAndFilenameSize(const SW8* path_, W16& pathSize_, W16& filenameSz_)
{
	// recuar em path_ para apanhar só o diretorio base
	const W16 totalPathSz = strlen(path_);
	if (totalPathSz == 0) return;

	const SW8* p = path_ + totalPathSz - 1;
	while (p!=path_)
	{
		if (*p=='/' || *p=='\\') break;
		p--;
	}
	pathSize_ = p - path_ + 1;
	filenameSz_ = totalPathSz - pathSize_;
}
W32      fs_getClusterSizeInBytes()
{
	return fs_cluster_LBASize*512;
}
W0       fs_spaceClusters(W32& total_, W32& ocuppied_, W32& free_) // em páginas de 512K 
{
	const W16  num = (fs_type==FS_FAT16)?(512>>1):(512>>2);
	total_ = fs_fat_LBASize*num;
	free_ = fs_countFreeClusters_();
	ocuppied_ = total_ - free_;
}
W0       fs_space(W32& total_, W32& ocuppied_, W32& free_) // em páginas de 512K 
{
	const W16  num = (fs_type==FS_FAT16)?(512>>1):(512>>2);
	total_ = fs_fat_LBASize*num;
	free_ = fs_countFreeClusters_();
	ocuppied_ = total_ - free_;
	
	total_*=fs_cluster_LBASize;
	ocuppied_*=fs_cluster_LBASize;
	free_*=fs_cluster_LBASize;
}
W32      fs_bytesToClusters(W32 bytes_)
{
	const W32 lbas = (bytes_>>9)+((bytes_%512)?1:0);
	return (lbas/fs_cluster_LBASize)+((lbas%fs_cluster_LBASize)?1:0);
}

FS_FLAGS fs_detectVolume_(const W8 partNum_, 
						  const W1 initFS_, 
						  FS_TYPE* type_ = 0,
						  SW8* volName_=0)
{ 
	if (initFS_)
	{
		DEBUG("\nfs:reinit MBR:%u", partNum_)
	}
	if (volName_)
		*volName_=0;

	FS_DEV_GETBLK(0, fs_buff);
	
	//printf("\n HERE1!!!!");

	if (((W16*)(fs_buff+0x01FE))[0]!=0xAA55) 
	{
		fatal(FS, FS_NO_MBR_SIGN);
	}
	MBRPart *part = (MBRPart*) (fs_buff + MBR_START) + partNum_;
	if (!(part->type==0xC||part->type==0x6||part->type==0x4||part->type==0xE)) return FS_NO_MBR_PART;
	
	const W32 currLBAStart = fs_LBAStart;

	if (part->type >= MBR_HAVE_LBA)
	{
		fs_LBAStart = part->LBAStart;
	}
	else 
	{
		fs_LBAStart = chsToLBA(
								MBR_EXTRACT_C(part->CHSStart),
								MBR_EXTRACT_H(part->CHSStart),
								MBR_EXTRACT_S(part->CHSStart),
								0,//DEV_TOTAL_HEADS, 
								0//DEV_TOTAL_SECTPERCILS
								);
	}
	
	// teste
	//int i=0;
	//while(1)
	//{
	//	memset(fs_buff, 0, 512);
	//	W8 sdError = DEV_RD_BLOCK(i,fs_buff);
	//	
	//	if (sdError!=RES_OK)
	//	{
	//		printf("\n\n!!!!SD ERRROR!!!!: %i lastR1reply:%02X", sdError, sd_lastR1reply);
	//	}else
	//	{
	//		prnBuff(fs_buff, 32);
	//	}
	//	DELAY_MS(3000); i++;
	//}
	//while(1){}

	// senao for para reinicializar o fs, deixar como estava
	if (!initFS_) fs_LBAStart = currLBAStart;

	W8 sdError = FS_GETBLK(0,fs_buff);
	if (sdError!=RES_OK)
	{
		printf("\n\n!!!!FS SD ERRROR!!!!: %i lastR1reply:%02X", sdError, sd_lastR1reply);
	}
	BootHeader* b = (BootHeader*) fs_buff;


	FS_TYPE type = b->detectFatType();

	DEBUG("\nFAT%i ", (type==FS_FAT12)?12: (type<<3));

	// detetar se FAT32
	if (type==FS_FAT_NO) fatal(FS,FS_NO_VALID_FAT);
	if (type==FS_FAT12) fatal(FS,FS_IS_FAT12);
	if (!(type==FS_FAT16||type==FS_FAT32))  fatal(FS,FS_NO_VALID_FAT);
		

	if (type_) *type_ = type;
	if (initFS_)
	{
		//printf("\n HERE3!!!!");
		fs_type = type;
		fs_endClusterIdx = (fs_type==FS_FAT16)? 0xFFFF : 0x0FFFFFFF;
		fs_endInvalidIdx = (fs_type==FS_FAT16)? 0xFFF7 : 0x0FFFFFF7;

		// assume que numBytesPerSector = 512, 1 sector = 1 LBA

		const W32 numSectorsPerFAT = (fs_type==FS_FAT32)? b->ext.dos7.numSectorsPerFAT: 
														  b->numSectorsPerFAT;

		fs_cluster_LBASize		= b->numSectorsPerCluster;
		fs_fat_LBAStart         = b->numSectorsReserved; // divisao por 512 bytes (sector/1LBA)
		
		if (fs_type==FS_FAT32)
		{
			fs_fatBackup_LBAStart   = b->ext.dos7.numSectorBackup;
			//<!> nao testado backup

			fs_rootDir_cluster      = b->ext.dos7.numClusterRootDir;
			fs_rootDir_LBASize		= 0; // nao existe
			fs_fat_LBASize          = b->ext.dos7.numSectorsPerFAT; 
			fs_allocs_LBAStart      = fs_fat_LBAStart + (b->numFATs) * fs_fat_LBASize;
		}
		else
		{
			fs_fatBackup_LBAStart   = fs_fat_LBAStart + fs_fat_LBASize;
		
			const W32 p0 = b->numEntriesAtRootDir<<5;
			fs_rootDir_LBASize  = (p0>>9) + ((p0%512)?1:0);
			fs_allocs_LBAStart  = fs_fat_LBAStart +
								  ((b->numFATs) * b->numSectorsPerFAT) 
								  + fs_rootDir_LBASize;
			fs_fat_LBASize      = b->numSectorsPerFAT; 
		}
		fs_currMBR = partNum_;
	}
	
	if (volName_)
	{
		b->copyVolumeName(volName_);
	}
	
	fs_invalidate_fat_cache_();
	return FS_OK;
}


// retorna 1 se continua
inline W32  fs_getClusterLBA_(const W32 clusterIdx_)
{
	// isto é só para FAT16
	return fs_allocs_LBAStart  + (clusterIdx_-2) * fs_cluster_LBASize;
}
// devolve o valor do proximo cluster
// <lança o valor terminador se o final e FS_FAIL>
FS_FLAGS    fs_getNextClusterIdx_(W32& currClusterIdx_)
{
	#ifdef FS_DEBUG_NEXT_CLUSTER
		FS_DEBUG_LOG("\n[%08X",currClusterIdx_);
	#endif
	const W32 relativeIdx = fs_load_fat_cache_for_(currClusterIdx_);
	
	FAT_TABLE_GET_VALUE_RELATIVE(relativeIdx, currClusterIdx_)
	
	#ifdef FS_DEBUG_NEXT_CLUSTER
		FS_DEBUG_LOG(">>%08X]",currClusterIdx_);
	#endif

	if ( FS_CLUSTERIDX_IS_TERMINATOR(currClusterIdx_))
		return FS_FAIL;

	return FS_OK;
}
// devolve o ultimo cluster duma cadeia (o marcado com o terminador)
W32         fs_getLastClusterIdx_(W32 start_)
{
	W32 prev;
	while(1)
	{
		prev = start_; // isto é só para evitar que retorne o terminador
		if (fs_getNextClusterIdx_(start_)==FS_FAIL) break;
	}
	return prev;
}	

FS_FLAGS    fs_searchFreeCluster_(W32& freeCluster_)
{
	struct FS_SearchFree
	{
		W1 found;
		W32 freeCluster;
		FS_SearchFree()
		{
			found = false;
			freeCluster = 0;
		}
		W1 run(W32 clusterIdx_, W32 clusterValue_)
		{
			if (clusterValue_==0) 
			{ 
				found =true; 
				freeCluster = clusterIdx_; 
				return true; 
			}
			return false;
		}
	} action;

	if (fs_type==FS_FAT16)
		FS_FATIterator<W16,FS_SearchFree>::run(action);
	else
		FS_FATIterator<W32,FS_SearchFree>::run(action);

	if (!action.found) return FS_FAIL;  
	freeCluster_ = action.freeCluster;
	return FS_OK;
}
W0          fs_setClusterPageToZero_(W32 cluster_)
{
	W32 LBA = fs_getClusterLBA_(cluster_);
	memset(fs_buff, 0, 512);
	for (W8 i=0; i< fs_cluster_LBASize; i++, LBA++)
	{
		FS_SAVEBLK(LBA, fs_buff);
	}
}
W0          fs_setFatClusterValue_(W32 clusterIdx_, W32 value_)
{
	const W32 relativeIdx = fs_load_fat_cache_for_(clusterIdx_);
	
	if (fs_type==FS_FAT16)
		((W16*)FS_FAT_BUFFER)[relativeIdx]=value_;
	else
		((W32*)FS_FAT_BUFFER)[relativeIdx]=value_;
	
	fs_save_fat_cache_();
}
W0          fs_linkClusterRange_(W32 clusterFirstIdx_, W32 clusterEndIdx_)
{
	fs_load_fat_cache_for_(clusterFirstIdx_);

	for (W32 i=clusterFirstIdx_; i< clusterEndIdx_; i++)
	{

		// se o buffer FAT mudou para este cluster
		// guardar o buffer FAT
		W16 newFatNum, relativeIdx ;
		fs_get_fat_cache_details_(i, newFatNum, relativeIdx);
		
		
		#ifdef FS_DEBUG_CLUSTERLINK
			printf("\nfslink: clust:%i fat[curr:%i/new:%i], clustRelIdx:%i", 
					i, fs_currFatNum, newFatNum, relativeIdx);
		#endif
		
		if (newFatNum!=fs_currFatNum)
		{
			fs_save_fat_cache_();
			#ifdef FS_DEBUG_CLUSTERLINK
				printf(" ** saved");
			#endif
		}
		// carregar se necessária novo buffer FAT
		relativeIdx = fs_load_fat_cache_for_(i);

		// alterar <!> FAT12?
		if (fs_type==FS_FAT32)
			((W32*)FS_FAT_BUFFER)[relativeIdx]=i+1;
		else if (fs_type==FS_FAT16)
			((W16*)FS_FAT_BUFFER)[relativeIdx]=i+1; 
		else
		{
		}
	}
	fs_save_fat_cache_();
	#ifdef FS_DEBUG_CLUSTERLINK
		printf(" ** saved");
	#endif
	// guardar terminador
	fs_setFatClusterValue_(clusterEndIdx_, fs_endClusterIdx);
}

FS_FLAGS    fs_allocCluster_(W32& startCluster_, W1 setZeros_=false)
{
	// procurar o ultimo cluster
	W32 firstCluster = startCluster_, prevCluster = firstCluster;
	while (fs_getNextClusterIdx_(firstCluster)==FS_OK)
	{
		prevCluster = firstCluster;
	}
	W32 newCluster;
	if (fs_searchFreeCluster_(newCluster)) return FS_FAIL;

	// atribuir o novo cluster como FIM
	fs_setFatClusterValue_(newCluster, fs_endClusterIdx);

	// atribuir no anterior cluster como o NEWCLUSTER
	fs_setFatClusterValue_(prevCluster, newCluster);

	// limpar a pagina de diretorio/ficheiro no novo cluster
	if (setZeros_) fs_setClusterPageToZero_(newCluster);

	startCluster_ = newCluster;
	fs_invalidate_fat_cache_();
	return FS_OK;
}
FS_FLAGS    fs_allocNewCluster_(W32& newCluster_, W1 setZeros_=false)
{
	if (fs_searchFreeCluster_(newCluster_)) return FS_FAIL;
	// atribuir o novo cluster como FIM
	fs_setFatClusterValue_(newCluster_, fs_endClusterIdx);

	// limpar a pagina de diretorio/ficheiro no novo cluster
	if (setZeros_) fs_setClusterPageToZero_(newCluster_);

	fs_invalidate_fat_cache_();
	return FS_OK;
}
FS_FLAGS    fs_allocNewClusters_(W32 startingCluster_, 
							     W32 num_, W32& allocated_,
							     W32* firstAllocatedCluster_=0,
							     W32* lastAllocatedCluster_=0,
							     W1 initOnClusterPage_=true,
							     W1 setEndMarker_ = true)
{
	W32 nextValue;
	// verificar se o cluster de entrada tem o marcador de fim
	W32 prevClusterRel = fs_load_fat_cache_for_(startingCluster_);
	FAT_TABLE_GET_VALUE_RELATIVE(prevClusterRel, nextValue);	
	if ( nextValue != fs_endClusterIdx)
		return FS_FAIL;

	const W32 numEntriesPerFat = FAT_TABLE_GET_NUMBER_PER_PAGE();
	const W32 toAlloc = num_;
	W16   startPage, prevPage = fs_currFatNum, pcr;
	W32   lastAssignedCluster = startingCluster_, absCluster;
	W16   nextClusterRel;

	if (initOnClusterPage_)
	{
		absCluster     = startingCluster_+1;
		startPage      = FAT_TABLE_GET_FATPAGE_FOR_CLUSTER(absCluster);	
		nextClusterRel = FAT_TABLE_GET_RELATIVE_CLUSTER(absCluster, startPage);
		pcr = nextClusterRel;
	}
	else
	{
		absCluster = 0;
		startPage = 0;
		nextClusterRel = 0;
		pcr = 0;
	}

	W1 first = true;
	for (W16 page=startPage; page< fs_fat_LBASize; page++)
	{
		W1 pageChanged = false;
		FS_GETBLK(fs_fat_LBAStart + page, FS_FAT_BUFFER);
		for (W16 rc=pcr; rc< numEntriesPerFat; rc++, absCluster++)
		{
			const W32 value =  (fs_type==FS_FAT32)?			
								(((W32*)FS_FAT_BUFFER)[rc]):	
								(((W16*)FS_FAT_BUFFER)[rc]);

			if (value != 0) continue;
			
			if (first&&firstAllocatedCluster_)
			{
				*firstAllocatedCluster_ = absCluster;
				first = false;
			}

			// se a entrada estiver livre
			lastAssignedCluster = absCluster;

			
			if (prevPage==page)
			{
				// se o anterior ficar na mesma página, apenas guardar
				// o valor (do próximo) no currente buffer FAT
				FAT_TABLE_SET_VALUE_RELATIVE(prevClusterRel, absCluster);
				// inidicar para gravar a página apenas quando acabar
				pageChanged = true;
			}
			else
			{
				// se o anterior ficar noutra página, 
				// carregar a página anterior
				// guardar o valor (do próximo), guardar a página anterior
				// e voltar a recarregar a página currente
				FS_GETBLK(fs_fat_LBAStart + prevPage, FS_FAT_BUFFER);
				FAT_TABLE_SET_VALUE_RELATIVE(prevClusterRel, absCluster);
				FS_SAVEBLK(fs_fat_LBAStart + prevPage, FS_FAT_BUFFER);
				FS_GETBLK(fs_fat_LBAStart + page, FS_FAT_BUFFER);
			}
			prevPage = page;
			prevClusterRel = rc; 

			num_--;
			if (num_ == 0) break;
		}

		// se a página currente mudou gravar alterações
		if (pageChanged)
		{
			FS_SAVEBLK(fs_fat_LBAStart + page, FS_FAT_BUFFER);
		}
		if (num_==0) break;
		// indicar que a entrada relativa agora é 0 na próxima página
		pcr = 0;
	}

	// invalidar a cache FAT porque o buffer teve a ser usado
	// sem as funções fs_load/save_fat
	fs_invalidate_fat_cache_();
	// usar agora estas funções fs_load/save_fat para colocar 
	// o último cluster com o terminador de fim
	if (setEndMarker_)
	{
		prevClusterRel = fs_load_fat_cache_for_(lastAssignedCluster);
		FAT_TABLE_SET_VALUE_RELATIVE(prevClusterRel, fs_endClusterIdx);
		fs_save_fat_cache_();
	}
	
	allocated_ = toAlloc - num_;
	if (allocated_)
	{
		if (lastAllocatedCluster_) *lastAllocatedCluster_ = lastAssignedCluster;
	}
	return FS_OK;
}

FS_FLAGS    fs_getNextClusterIdxForRW_(W32& currClusterIdx_, W8 op_, SW8 minPreAlloc=-1)
{
	W32 prevClusterBackup = currClusterIdx_;
	if (op_!=FS_OP_WRITE)  return fs_getNextClusterIdx_(currClusterIdx_);

	if (fs_getNextClusterIdx_(currClusterIdx_)==FS_OK) 
	{
		#ifdef FS_DEBUG_NEXT_WR_CLUSTER
			FS_DEBUG_LOG("\nNext WR cluster:", currClusterIdx_);
		#endif
		return FS_OK;
	}
	currClusterIdx_ = prevClusterBackup;

	if (minPreAlloc==-1)
	{
		#ifdef FS_DEBUG_NEXT_WR_CLUSTER
			FS_DEBUG_LOG("\nNext NEW WR cluster for %Xh",currClusterIdx_);
		#endif

		if (fs_allocCluster_(currClusterIdx_, false)==FS_FAIL) 
		{
			#ifdef FS_DEBUG_NEXT_WR_CLUSTER
				FS_DEBUG_LOG("cant alloc!",0);
			#endif
			return FS_FAIL;
		}
		#ifdef FS_DEBUG_NEXT_WR_CLUSTER
			FS_DEBUG_LOG("its %X", currClusterIdx_);
		#endif
	}
	else
	{
		W32 allocated;
		if (fs_allocNewClusters_(currClusterIdx_, minPreAlloc, allocated)==FS_FAIL) 
		{
			#ifdef FS_DEBUG_NEXT_WR_CLUSTER
				FS_DEBUG_LOG("cant alloc!",0);
			#endif
			return FS_FAIL;
		}
		
		#ifdef FS_DEBUG_NEXT_WR_CLUSTER
			FS_DEBUG_LOG("\nNext NEW WR cluster begins: %X #i",currClusterIdx_,allocated);
		#endif
	}

	return FS_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Funções auxiliares de iteração de diretório ///////////////////////////
//////////////////////////////////////////////////////////////////////////
FS_FLAGS  fs_itfx_search_(FatEntry* entry_, W32 entryLBA_, W16 entryOffset_, W0* search_)
{
	FS_Search* search = (FS_Search*) search_;
	// procurar apenas entrada livre
	switch(search->searchType)
	{
		case FS_SearchEntry_Free:
		{
			if (entry_->isFree()) goto found;
		
			return FS_ITERATE_CONTINUE;
		}
		break;
		case FS_SearchEntry_Cluster:
		{
			if (!(entry_->isValid())) return FS_ITERATE_CONTINUE;
			if (entry_->getCluster()==search->in.clusterToMatch) goto found;

			return FS_ITERATE_CONTINUE;
		}
		break;
		case FS_SearchEntry_Name:
		{
			if (!(entry_->isValid())) return FS_ITERATE_CONTINUE;
			SW8 cstr[13];
			entry_->getShortName(cstr);
			upperStr(cstr);
			//printf("\nsearch: %s =  %s?", cstr,search->in.nameToMatch);
			
			if (strcmp(search->in.nameToMatch, cstr)!=0) 
				return FS_ITERATE_CONTINUE;
			goto found;
		}
		break;
	}
	return FS_ITERATE_STOP;

found:
	search->found = 1;
	search->isDir = entry_->isDir();
	search->res.fizeSize = entry_->fileSize;
	search->res.foundCluster = ((entry_->clusterHi)<<16)| entry_->clusterLow;
	search->res.entryLBA = entryLBA_;
	search->res.entryOffset = entryOffset_;
	FS_DEBUG_LOG("\nfound at lba:%xh off:%xh", entryLBA_, entryOffset_)
	return FS_ITERATE_STOP;
}
FS_FLAGS  fs_itfx_prnEntry(FatEntry* entry_, W32 entryLBA, W16 entryOffset_, W0* arg_)
{
	if (!(entry_->isValid())) return FS_ITERATE_CONTINUE;
	SW8 name[13];
	entry_->getShortName(name);
	DEBUG("\n %s %13s %010ib 0x%02X {%04X", 
			entry_->isDir()?"[DIR]":"     ", 
			name, 
			entry_->fileSize, 
			entry_->attribs,
			((W32)(entry_->clusterHi)<<16)+(W32)(entry_->clusterLow));
	
	#ifdef FS_PRINTENTRY_PRINTS_ALL_CLUSTERS
 	if ((!entry_->isParent()) && (!entry_->isSelf()))
	{
		W32 clusterCount=1, count=0, prev, next = entry_->clusterLow;
			prev=next;
		while(1)
		{
			FS_FLAGS res = fs_getNextClusterIdx_(next);
			clusterCount++;
			FS_DEBUG_LOG(">%04X",next)

			//if (prev==next-1)
			//{
			//	count++;
			//	prev = next ;
			//	continue;
			//}
			//if (count)
			//	FS_DEBUG_LOG("+%i>%i",count,next)
			//else
			//	FS_DEBUG_LOG(">%i",next)
			//prev = next;
			//count = 0;
			if (res==FS_FAIL) break;
		}
		FS_DEBUG_LOG(" #%i",clusterCount)
	}
		#endif
	FS_DEBUG_LOG("}",0);
	
	return FS_ITERATE_CONTINUE;
}
FS_FLAGS  fs_itfx_count_(FatEntry* entry_, W32 entryLBA, W16 entryOffset_, W0* cnt_)
{
	if (!(entry_->isValid())) return FS_ITERATE_CONTINUE;
	 (*((W32*)cnt_))++;
	return FS_ITERATE_CONTINUE;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Iteração de diretório /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
FS_FLAGS  fs_iterateEntries_(W32 LBA, FS_ITERATOR_FUNCTION fx_, W0* fxArgs_)
{
	FS_GETBLK(LBA, fs_buff);
	FatEntry* entry = (FatEntry*)fs_buff;
	W16 entryOffset = 0;
	for (W16 e=0; e< (512/sizeof(FatEntry)); e++, entry++, entryOffset+=sizeof(FatEntry))
	{
		//DEBUG_PRINT("\nfs: chamando a fxit_");
		if (fx_(entry, LBA, entryOffset, fxArgs_)==FS_ITERATE_STOP) 
			return FS_ITERATE_STOP;	
	}
	return FS_ITERATE_CONTINUE;
}
FS_FLAGS  fs_iterateDir_(W32 clusterIdx_, FS_ITERATOR_FUNCTION fx_, W0* fxArgs_)
{
	const W1 isOlderRootDir = FS_IS_OLDER_DIRROOT(clusterIdx_);
	
	
	//FS_DEBUG_LOG("\n***** iteracao diretorio cluster %i", clusterIdx_)
	//DEBUG("\nfs_it %i %i %i", fs_type, clusterIdx_, isOlderRootDir);
	if (isOlderRootDir)
	{
		W32 LBA = fs_fat_LBAStart + fs_fat_LBASize*2;
		for (W16 i=0; i< fs_rootDir_LBASize; i++, LBA++)
		{
			//FS_DEBUG_LOG("\n**** ROOT DIR FAT16 LBA %i", LBA)
			if (fs_iterateEntries_(LBA, fx_, fxArgs_)==FS_ITERATE_CONTINUE) 
				continue;
			return FS_FOUND;
		}
	}
	else
	{
		do 
		{
			W32 LBA = fs_getClusterLBA_(clusterIdx_);
			// iterar tantos LBAs quanto tenha um cluster
			for (W16 i=0; i< fs_cluster_LBASize; i++, LBA++)
			{
				if (fs_iterateEntries_(LBA, fx_, fxArgs_)==FS_ITERATE_CONTINUE) continue;
				return FS_FOUND;
			}
		}
		while (fs_getNextClusterIdx_(clusterIdx_)==FS_OK);
	}
	return FS_NOT_FOUND;
}
W32       fs_countDirFiles_(W32 clusterIdx_)
{
	W32 counter=0;
	fs_iterateDir_(clusterIdx_, fs_itfx_count_, &counter);
	return counter;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Interface com FSNode //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Handler
VL_PACK_BEGIN
struct VL_PACK_MIDDLE FSNode
{
	#define FS_NODE_PARTNUM_ST   0 
	#define FS_NODE_PARTNUM_END  1 
	#define FS_NODE_IS_VALID     2 
	#define FS_NODE_IS_DIR       3 
	W32 parDirCluster, cluster;
	W8  info;
	W8  isSequential;
	union
	{
		VL_PACK_BEGIN
		struct VL_PACK_MIDDLE
		{	
			W32 fileSize; 
			W32 absoluteOffset;
			W32 currCluster;			// ponteiro de leitura/escrita	
			W8  currClusterPart;		// qual setor 
			W16 currRelativeOffset;		// offset relativo ao setor
		} file;
		VL_PACK_END
		VL_PACK_BEGIN
		struct VL_PACK_MIDDLE
		{
			W32 fileCount;
		} dir;
		VL_PACK_END
	};
	FSNode() { info=0; isSequential=0;}
		 
	W0 setValid(W32 parDirCluster_, const W1 true_=true) { parDirCluster = parDirCluster_; info = W8_BIT_SET(info,FS_NODE_IS_VALID,true_); }
	W0 setAsDir(const W1 true_)      { info = W8_BIT_SET(info,FS_NODE_IS_DIR,true_); }
	W0 setPartitionNum(const W8 num_){ info = SET_BITS(info, FS_NODE_PARTNUM_ST, FS_NODE_PARTNUM_END, num_);}
	W0 setInCurrPartition()          { info = SET_BITS(info, FS_NODE_PARTNUM_ST, FS_NODE_PARTNUM_END, fs_currMBR);}
	W8 getPartitionNum()			 { return GET_BITS(info,FS_NODE_PARTNUM_ST,FS_NODE_PARTNUM_END); };
	W1 isValidFile()				 { return W8_BIT_GET(info,FS_NODE_IS_VALID)&&(!W8_BIT_GET(info,FS_NODE_IS_DIR)); };
	W1 isValidDir()					 { return W8_BIT_GET(info,FS_NODE_IS_VALID)&&W8_BIT_GET(info,FS_NODE_IS_DIR); };
	W1 isValid()					 { return W8_BIT_GET(info,FS_NODE_IS_VALID); };
	
	
	W0 print()
	{
		#ifdef FS_FSNODE_HAS_PRINT
		printf("\n Parent Cluster: 0x%08X", parDirCluster);
		printf("\n        Cluster: 0x%08X", cluster);
		printf("\n        isValid: %1i", W8_BIT_GET(info,FS_NODE_IS_VALID));
		printf("\n          isDir: %1i", W8_BIT_GET(info,FS_NODE_IS_DIR));
		printf("\n      partition: %i", getPartitionNum());
		if (W8_BIT_GET(info,FS_NODE_IS_DIR))
		{
			printf("\nAs dir:");
			printf("\n Entry count: ", dir.fileCount);
		}
		else
		{
			printf("\nAs file:");
			printf("\n             Size: %i ", file.fileSize);
			printf("\n       Abs offset: %i ", file.absoluteOffset);
			printf("\n       Rel offset: %i ", file.currRelativeOffset);
			printf("\n     Curr cluster: 0x%08X ", file.currCluster);
			printf("\n     Curr rel LBA: %i ", file.currClusterPart);
			printf("\n  Curr rel offset: %i ", file.currRelativeOffset);
		}
	#endif
	}
};
VL_PACK_END
	
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Verifica se o Node está noutra partição e se sim //////////////////////
// reinicializa o Filesystem                        //////////////////////
//////////////////////////////////////////////////////////////////////////
FS_FLAGS  fs_checkNode_(FSNode* n_)
{
	const W8 nodePart = n_->getPartitionNum();
	//printf("\nnode esta' em particao %i", nodePart);
	if (fs_currMBR==nodePart) return FS_OK;
	FS_FLAGS status = fs_detectVolume_(nodePart, true, 0);
	if (status!=FS_OK) return status;
	return FS_OK;
}
FS_FLAGS  fs_getNodeEntry(FSNode* node_, W32& LBA_, W16& LBAOffset_)
{
	if (node_->parDirCluster==-1) 
	{
		#ifdef FS_DEBUG_GET_NODE_ENTRY
			printf("\nfs_getNodeEntry: parent dir cluster not found!");
		#endif
		return (FS_FLAGS)FS_FAIL;
	}
	FS_Search search(FS_SearchEntry_Cluster);
		search.in.clusterToMatch = node_->cluster;
	
	fs_iterateDir_(node_->parDirCluster, fs_itfx_search_, &search);

	if (!search.found) return FS_FAIL;

	LBA_ = search.res.entryLBA;
	LBAOffset_ = search.res.entryOffset;


	return FS_OK;
}
FS_FLAGS  fs_getFilenameExt(const SW8* fn_, SW8* ext_)
{
	W32 flen = strlen(fn_);
	ext_[0] = ' ';
	ext_[1] = ' ';
	ext_[2] = ' ';
	if (flen==0) return FS_FAIL;
	SW8* s = (SW8*)fn_ + flen - 1;
	while(s!=fn_)
	{
		if (*s == '.')
		{
			s++;
			W8 len = fn_ + flen - s;
			if (len>3) len=3;
			for (W8 i=0; i< len; i++, s++)
				ext_[i]=toupper(*s); 
			return FS_OK;
		}
		s--;
	}
	return FS_FAIL;
}
W1        fs_isRootDirNode(FSNode* node_=0)
{
	if (fs_type==FS_FAT32)
	{
		if (node_->cluster == fs_rootDir_cluster) return true;
		return false;
	}
	else
	{
		if (node_->cluster == 0) return true; 
		return false;
	}
}
FS_FLAGS  fs_getRootDirNode(FSNode* node_=0, W8 part_=0)
{
	if ( fs_currMBR != part_ ) 
		if (FS_OK!=fs_detectVolume_(part_, true)) 
			return FS_FAIL;
	node_->cluster		 = (fs_type==FS_FAT32)? fs_rootDir_cluster:0;
	node_->setValid(-1,true);
	node_->setAsDir(true);
	node_->setPartitionNum(part_);
	node_->dir.fileCount = fs_countDirFiles_(node_->cluster);
	return FS_OK;
} 
FS_FLAGS  fs_iterateDir(FSNode* node_, FS_ITERATOR_FUNCTION fx_, W0* fxArgs_)
{
	//printf("\nnode valid?");
	if (!node_->isValidDir()) return FS_FAIL;
	fs_checkNode_(node_);
	//printf("\niterating...");
	return fs_iterateDir_(node_->cluster, fx_, fxArgs_);
}
FS_FLAGS  fs_search(FSNode* node_, const SW8* name_)
{
	SW8 name[13];
	if (!node_->isValidDir()) return FS_FAIL;
	fs_checkNode_(node_);

	memcpy(name, name_, 13);
	upperStr(name);

	FS_Search search(FS_SearchEntry_Name);
		search.in.nameToMatch = name;
	
	W32 parentDirCluster = node_->cluster;
	fs_iterateDir_(parentDirCluster, fs_itfx_search_, &search);
	
	//printf("FSSEARCH 1!!!!");
	if (!search.found)
	{
		return FS_FAIL;
	}
	//printf("FSSEARCH 2!!!!");

	memset(node_, 0, sizeof(FSNode));
	node_->setValid(parentDirCluster, true);
	node_->cluster = search.res.foundCluster;
	node_->setInCurrPartition();
	node_->setAsDir(search.isDir);
	if (node_->isValidDir()) 
		node_->dir.fileCount = fs_countDirFiles_(node_->cluster);
	else
	{
		node_->file.fileSize = search.res.fizeSize;
		node_->file.currCluster = node_->cluster;
		node_->file.absoluteOffset = 0;
		node_->file.currClusterPart = 0;
		node_->file.currRelativeOffset = 0;
	}
	return FS_OK;
}
FS_FLAGS  fs_getNodeAtAbsPath(FSNode* node_, W8 partNum_, SW8* path_, W16 pathSz_=0)
{ 
	if (path_==0) return FS_FAIL;
	if (*path_==0) return FS_FAIL;

	if (pathSz_==0) pathSz_ = strlen(path_);
	
	fs_getRootDirNode(node_, partNum_);
	if ((path_[1]==0)||(pathSz_==1))
	{
		if (path_[0]=='/' || path_[0]=='\\') return FS_OK;
	}

	SW8* path = path_;

	while(pathSz_!=0)
	{
		pathSz_--;
		const W8 ch = *path; 
		if (ch=='/' || ch=='\\')
		{
			const W8  nameLen = path-path_;
			if (nameLen)
			{
				SW8* name = new SW8[nameLen+1];
				memcpy(name, path_, nameLen);
				name[nameLen]=0;
				const FS_FLAGS found = fs_search(node_, name);
				//fs_itfx_prnEntry(&node_, fs_printEntry, 0);
				delete[] name;
			
				if ((found!=FS_OK) || !node_->isValidDir()) 
					return FS_FAIL;
			}
			path_ = path+1;
		}
		path++;
	}
	if (path==path_) return FS_OK;
	if (fs_search(node_, path_)==FS_OK) return FS_OK;
	
	node_->setValid(false); 
	return FS_FAIL;
	
		
}

FS_FLAGS  fs_reserveClusters_(FSNode* node_,  W32 numClusters_, W32& allocated_, W32* beginCluster_=0)
{
	if (!node_->isValidFile()) return FS_FAIL;
	fs_checkNode_(node_);

	W32 lastCluster, beginCluster = fs_endClusterIdx;
	allocated_ = 0;
	// procurar ultimo cluster apartir do ponteiro
	const W32 last = (FS_CLUSTERIDX_IS_TERMINATOR(node_->file.currCluster))?
		fs_getLastClusterIdx_(node_->cluster):
		fs_getLastClusterIdx_(node_->file.currCluster);
	if (fs_allocNewClusters_(last, numClusters_, allocated_, &beginCluster , &lastCluster, true, false)==FS_FAIL)
		return FS_FAIL;

	// se nao conseguiu alocar linearmente apartir do 1º voltar ao inicio
	// da tabela e ver se pode alocar mais
	if (allocated_==numClusters_) goto skip;
	{
		W32 allocated2 = 0;
		if (fs_allocNewClusters_(last, lastCluster, allocated2, (allocated_==0)? &beginCluster:0 , &lastCluster, false, false)==FS_FAIL)
			return FS_FAIL;
	
		allocated_+=allocated2;
	}
skip:

	// atualizar 
	W32 lastRel = fs_load_fat_cache_for_(lastCluster);
	FAT_TABLE_SET_VALUE_RELATIVE(lastRel, fs_endClusterIdx);
	fs_save_fat_cache_();

	if (beginCluster_) * beginCluster_ = beginCluster;
	return FS_OK;
}

// atualiza a entrada de diretório consoante o que tem NODE
FS_FLAGS  fs_updateNodeEntry_(FSNode* node_, SW8* newName_=0, W1 setFree_=false)
{
	#ifdef FS_DEBUG_UPDATE_NODE_ENTRY
		printf("\nNode update:");
	#endif
	W32 LBA;
	W16 relativeOffset;
	if ( fs_getNodeEntry(node_, LBA, relativeOffset) != FS_OK)
	{
		FS_DEBUG_LOG("\nnot found!",0);
		return FS_FAIL_TO_UPDATE;
	}

	FS_GETBLK(LBA, fs_buff);
	#ifdef FS_DEBUG_UPDATE_NODE_ENTRY
		printf("\nDirEntry LBA: %Xh+%i entries:\n", LBA, relativeOffset); 
		FatEntry *e = (FatEntry*)(fs_buff);
		for (W16 i=0; i< 512/32; i++, e++)
		{
			e->print();
		}
	#endif

	FatEntry *entries = (FatEntry*) fs_buff;
	FatEntry *entry = (FatEntry*) (fs_buff+relativeOffset);

	if (setFree_)
	{
		entry->setFree();
	}
	else
	{
		entry->fileSize = node_->file.fileSize;
		#ifdef FS_DEBUG_UPDATE_NODE_ENTRY
			printf("\nsz up:%i", entry->fileSize);
		#endif
		if (newName_)
		{
			#ifdef FS_DEBUG_UPDATE_NODE_ENTRY
				printf("\nName chg:'%s'", newName_);
			#endif
			entry->setShortName(newName_);
		}
	}
	FS_SAVEBLK(LBA, fs_buff);
	return FS_OK;
}
FS_FLAGS  fs_createEntry_(FSNode* dirNode_, 
						 const SW8* fileName_, 
						 const W1 isDir_,
						 W1 makeNewCluster_=true, 
						 W32 otherCluster_=0, W1 afterRootDir_=false)
{
	W32 newCluster = 0;
	FS_Search search(FS_SearchEntry_Free);

	FS_DEBUG_LOG("fs_createEntry - dir cluster:%u", dirNode_->cluster);

	// encontrar uma entrada livre no diretorio
	fs_iterateDir_(dirNode_->cluster, fs_itfx_search_, &search);

	// se nao for encontrada entrada livre,
	// é preciso criar uma nova página no diretório
	if (!search.found)
	{
		// se é o diretorio ROOT de FAT-16 não é possivel
		// esticar mais
		if ((fs_type ==FS_FAT16)&&(dirNode_->cluster==0)) 
		{
			return FS_ROOTDIR_NO_MORE_SPACE;
		}
		W32 dStCluster = dirNode_->cluster;
		if (fs_allocCluster_(dStCluster, dirNode_->isValidDir())==FS_FAIL)
			return FS_FAIL;
		if (fs_iterateDir_(newCluster, fs_itfx_search_, &search)==FS_NOT_FOUND)
			return FS_FAIL;
	}

	
	// estando certos que existe uma entrada livre, 
	// ...procurar um cluster para o ficheiro/diretorio
	if (makeNewCluster_)
	{
		// se é para ser a seguir ao ROOT DIR
		if ((fs_type==FS_FAT32) && afterRootDir_)
		{
			//<!>
		}
		else
		{
			if (fs_allocNewCluster_(newCluster,true)==FS_FAIL) return FS_FAIL;
			FS_DEBUG_LOG("fs_createEntry - cluster for new file:%u",newCluster);
		}
	}
	else
		newCluster = otherCluster_;

	// ...e guardar na entrada de diretorio
	FS_GETBLK(search.res.entryLBA, fs_buff); 
	FatEntry* entriesDir = (FatEntry*)(fs_buff); // debug
	FatEntry* entry = (FatEntry*)(fs_buff+search.res.entryOffset);
	memset(entry,0,sizeof(FatEntry));
	entry->setAs(isDir_);
	entry->setShortName((SW8*)fileName_);
	entry->setCluster(newCluster);
	entry->fileSize = 0;
	return (FS_FLAGS)FS_SAVEBLK(search.res.entryLBA, fs_buff);
}

//////////////////////////////////////////////////////////////////////////////////////////
FS_FLAGS  fs_seek_sequential(FSNode* node_, W32 bytes_)
{
	const W32 clusterSizeBytes =  (512*fs_cluster_LBASize);
	const W32 startCluster     = bytes_ / clusterSizeBytes;
	W32 bytesRemaining         = bytes_ % clusterSizeBytes;
		
	const W32 clusterPart = bytesRemaining/512;
	bytesRemaining  = bytesRemaining%512;
	
	node_->file.currCluster        =  node_->cluster + startCluster ;
	node_->file.currClusterPart    =  clusterPart;
	node_->file.currRelativeOffset =  bytesRemaining;
	node_->file.absoluteOffset     =  bytes_;
	return FS_OK;
}
FS_FLAGS  fs_rw_sequential_512B(FSNode* node_, W8* buffer_, FS_FLAGS op_)
{
	const W32 startLBA = fs_getClusterLBA_(node_->file.currCluster) + 
						 node_->file.currClusterPart;
	switch(op_)
	{
		case FS_OP_READ: // read
			FS_GETBLK(startLBA, buffer_);
		break;
		case FS_OP_WRITE: // write
			FS_SAVEBLK(startLBA, buffer_);
	}
	node_->file.currClusterPart++;
	node_->file.currRelativeOffset=0;
	node_->file.absoluteOffset+=512;
	if (node_->file.currClusterPart==fs_cluster_LBASize)
	{
		node_->file.currClusterPart = 0;
		node_->file.currCluster++;
	}
	return FS_OK;

}

FS_FLAGS  fs_rw_(FSNode* node_, W8* buffer_, W32 bytes_, W32& bytesWR_, FS_FLAGS op_, SW8 minPreAlloc_=-1)
{

	if ((node_->isSequential) && (op_==FS_OP_SEEK)) return fs_seek_sequential(node_,bytes_);
	FS_FLAGS status = FS_OK;
	//if (!node_->isValidFile()) return FS_FAIL;
	//fs_checkNode_(node_);
	
	//	W32 currCluster;			// ponteiro de leitura/escrita	
	//	W8  currClusterPart;		// qual setor 
	//	W8  currRelativeOffset;		// offset relativo ao setor
	W32 currClusterIdx  = node_->file.currCluster;
	W8  currClusterPart = node_->file.currClusterPart;
	W16 relativeOffset  = node_->file.currRelativeOffset;
	W32 absoluteOffset  = node_->file.absoluteOffset;
	W32 bytes           = bytes_;
	W32 LBA; 

	#ifdef FS_DEBUG_WR
		FS_DEBUG_LOG("\n*** fs:RW: acesso a offset %i, cluster 0x%X (parte %i/%i) offset relativo %i", 
						absoluteOffset, currClusterIdx, currClusterPart, fs_cluster_LBASize-1, relativeOffset);
	#endif

	// se está no fim e é uma escrita reservar mais clusters
	if ((op_==FS_OP_WRITE) && FS_CLUSTERIDX_IS_TERMINATOR(currClusterIdx))
	{
		W32 reservedClusters;
		minPreAlloc_ = (minPreAlloc_==-1)? 1: minPreAlloc_;
		const W32 extraClusters = MAX(minPreAlloc_, fs_bytesToClusters(bytes));
		
		#ifdef FS_DEBUG_WR
			FS_DEBUG_LOG("\nfs:WRITE: sem + clusters, a reservar %i clusters para %i bytes ",  extraClusters, bytes )
		#endif	

		if (FS_FAIL==fs_reserveClusters_(node_, extraClusters, reservedClusters, &currClusterIdx))
			return FS_FAIL;
		
		#ifdef FS_DEBUG_WR
			FS_DEBUG_LOG("\n - novo cluster de inicio 0x%X", currClusterIdx)
		#endif

		currClusterPart = 0; // 
		relativeOffset = 0;	 // nao devia ser necessario
	}

	//if (op_==FS_OP_SEEK)
	//	printf("\n  ----SEEK-----");
	//else
	//	printf("\n  ----R/W-----");

	if (bytes==0) goto fs_rw_done;

	while(1)
	{
		// startLBA: onde começa o cluster
		const W32 startLBA = fs_getClusterLBA_(currClusterIdx) ;
		LBA = startLBA + currClusterPart;
		const W32 LBAEnd = startLBA + fs_cluster_LBASize;
		
		// 1 cluster
		for (; LBA < LBAEnd; LBA++)
		{ 
			//DEBUG("\n-LBA:%u SETOR:%u REL:%u--------\n", LBA,currClusterPart, relativeOffset)
			
				// ... o numero de bytes nao pode ser maior do que sobra até ao fim do ficheiro
			W32 toTransfer;
			if (op_==FS_OP_READ) 
				toTransfer = VL_MIN(node_->file.fileSize-absoluteOffset, bytes); 
			else
				toTransfer = bytes;
			
			#ifdef FS_DEBUG_WR
				FS_DEBUG_LOG("\n*** fs:RW: a transfer %i bytes de %i para LBA %i (cluster 0x%X, parte %i)", toTransfer, bytes, LBA, currClusterIdx, currClusterPart);
			#endif

				// ... esse valor tem de ser nao maior do que resta do setor
			toTransfer     = VL_MIN( 512-(W32)relativeOffset, toTransfer );

			//printf("\n  clusterIdx:%08X | currClusterPart:%02i | LBA:%08X | bytes:%i toTransfer:%i", 
			//		currClusterIdx, currClusterPart, LBA, bytes, toTransfer);
				
			// se existirem bytes para transferir
			if ((op_!=FS_OP_SEEK) && (toTransfer!=0))
			{
				W8* target     = fs_buff + relativeOffset;
				switch(op_)
				{
					case FS_OP_READ: // read
						FS_GETBLK(LBA, fs_buff);
						memcpy(buffer_, target , toTransfer);
					break;
					case FS_OP_WRITE: // write
						if (toTransfer!=512) 
						{ 
							FS_GETBLK(LBA, fs_buff);
							if (buffer_)
								memcpy(target, buffer_, toTransfer); 
							else
								memset(target, 0, toTransfer); 
							FS_SAVEBLK(LBA, fs_buff);
						}
						else
						{
							FS_SAVEBLK(LBA, buffer_);
						}
					break;
				}
				if (buffer_) buffer_+= toTransfer;
			}
			bytes			-= toTransfer;
			absoluteOffset	+= toTransfer;

			// se acabaram os bytes
			if (bytes==0) 
			{
				// se prefez 512
				if (relativeOffset + toTransfer == 512)
				{
					// o offset relativo fica a 0 e tem 
					// rodar clusterPart
					currClusterPart++;
					relativeOffset	= 0;
					// se clusterPart rodou, fica a zero
					// e situado no proximo cluster
					if (currClusterPart==fs_cluster_LBASize)
					{
						
						if (fs_getNextClusterIdxForRW_(currClusterIdx,op_,minPreAlloc_)!=FS_OK) 
						{
							if (op_==FS_OP_WRITE) 
							{
								status = FS_NO_MORE_SPACE;
								goto fs_rw_done;
							}
						}
						//fs_getNextClusterIdxForRW_(currClusterIdx,FS_OP_READ);
						printf("\n**NEXT CLUSTER:%X",currClusterIdx);
						currClusterPart = 0;
					}
				}
				// senao prefez os bytes, o offset relativo
				// fica no mesmo sitio avançado por toTransfer
				else
				{
					relativeOffset += toTransfer;
				}
				goto fs_rw_done;	// sair do ciclo
			}

			// senao acabou decerteza preencheu um clusterPart
			relativeOffset	= 0;
			currClusterPart++;
			if (currClusterPart==fs_cluster_LBASize)
			{
				if (fs_getNextClusterIdxForRW_(currClusterIdx,op_,minPreAlloc_)!=FS_OK) 
				{
					if (op_==FS_OP_WRITE) 
					{
						status = FS_NO_MORE_SPACE;
						goto fs_rw_done;
					}
				}
				//fs_getNextClusterIdxForRW_(currClusterIdx,FS_OP_READ);
				//printf("\n**NEXT CLUSTER:%X",currClusterIdx);
				currClusterPart = 0;
			}
		} // fim de 1 cluster
	}
	
fs_rw_done:
	node_->file.currCluster        =  currClusterIdx;
	node_->file.currClusterPart    =  currClusterPart;
	node_->file.currRelativeOffset =  relativeOffset;
	node_->file.absoluteOffset     =  absoluteOffset;
	
	//printf("\n  ----\n  *** clusterIdx:%08X | currClusterPart:%02i | relOffset:%08X | absOffset: %08X\n  ---", 
	//				currClusterIdx, currClusterPart, relativeOffset, absoluteOffset);

	if ((op_==FS_OP_WRITE) && (absoluteOffset > node_->file.fileSize))
	{
		#ifdef FS_DEBUG_WR
			FS_DEBUG_LOG("\n*** fs:WRITE: definir tamanho total: %i bytes", absoluteOffset);
		#endif

		//atualizar o tamanho na entrada de diretório
		node_->file.fileSize =  VL_MAX(node_->file.fileSize, absoluteOffset);
		
	}
	#ifdef FS_DEBUG_WR
		FS_DEBUG_LOG("\n*** fs:RW: fim da operacao: offset absoluto %i, cluster 0x%X parte %i offset relativo %X", absoluteOffset, currClusterIdx, currClusterPart, relativeOffset);
	#endif


	bytesWR_ = bytes_-bytes;
	return status;
} 
FS_FLAGS  fs_seek(FSNode* node_, W32 offset_)
{
	if (!node_->isValidFile()) return FS_FAIL;
	fs_checkNode_(node_);

	if (offset_==0)
	{
		node_->file.absoluteOffset  = 0;
		node_->file.currCluster = node_->cluster;
		node_->file.currClusterPart = 0;
		node_->file.currRelativeOffset = 0;
		return FS_OK;
	}

	W32 bytesWr_ = 0;
	return fs_rw_(node_, 0, offset_, bytesWr_, FS_OP_SEEK);
}
FS_FLAGS  fs_read(FSNode* node_, W0* buffer_, W32 bytes_, W32& bytesWr_)
{
	bytesWr_ = 0;
	if (!node_->isValidFile()) return FS_FAIL;
	fs_checkNode_(node_);
	if (node_->file.absoluteOffset == node_->file.fileSize)
		return FS_FAIL;
	return fs_rw_(node_,  (W8*)buffer_, bytes_, bytesWr_, FS_OP_READ);
}
FS_FLAGS  fs_write(FSNode* node_, W0* buffer_, W32 bytes_, W32& bytesWr_, SW32 minPreAlloc_=-1)
{
	bytesWr_ = 0;
	if (!node_->isValidFile()) return FS_FAIL;
	fs_checkNode_(node_);
	return fs_rw_(node_, (W8*)buffer_, bytes_, bytesWr_, FS_OP_WRITE, minPreAlloc_);
}
FS_FLAGS  fs_tell(FSNode* node_, W32& bytes_)
{
	W32 bytesWr_ = 0;
	if (!node_->isValidFile()) return FS_FAIL;
	fs_checkNode_(node_);
	bytes_ = node_->file.fileSize;
	return FS_OK;
}
FS_FLAGS  fs_isEOF(FSNode* node_)
{
	fs_checkNode_(node_);
	if(node_->file.absoluteOffset>=node_->file.fileSize)
		return FS_FAIL;
	return FS_OK;
}

// crud de ficheiros
FS_FLAGS  fs_createNode_(const SW8* path_, FSNode* fileNode_, W1 isDir_, W8 part_=0, W1 openExisting_=false)
{ 
	FSNode pathNode; 
	W16 basePathSize, fileNameSize;	
	fs_getBasePathAndFilenameSize(path_, basePathSize, fileNameSize);
	
	// se o nome do ficheiro/diretorio nulo
	if (fileNameSize==0) return FS_FAIL;
	
	if (fs_getNodeAtAbsPath(&pathNode, part_, (SW8*)path_, basePathSize)) 
		return FS_FAIL;

	if (!(pathNode.isValidDir())) return FS_FAIL;

	// pesquisar ficheiro/dir, se já existir falhar
	*fileNode_ = pathNode;
	SW8* filename = (SW8*)path_ + basePathSize;
	FS_FLAGS fileFound = fs_search(fileNode_, filename);
	
	if (fileFound==FS_OK) 
	{
		if (openExisting_) return FS_OK;
		if (!openExisting_) return FS_FAIL;
	}
	else
	{
		if (openExisting_) return FS_FAIL;
		// criar o novo ficheiro em pathNode
		// encontrar uma entrada livre no diretorio pathNode
		if (fs_createEntry_(&pathNode, filename, isDir_)==FS_FAIL) 
			return FS_FAIL;
	}
	// confirmar que a entrada foi criada
	// e atribuir ao nó fileNode
	*fileNode_ = pathNode;
	fileFound  = fs_search(fileNode_, filename);
	if (fileFound!=FS_OK) return FS_FAIL;
	
	// se é um diretorio é preciso criar ("." e "..")
	if (isDir_ && !openExisting_)
	{
		if (fs_createEntry_(fileNode_, "..", isDir_, false, pathNode.cluster)==FS_FAIL) 
			return FS_FAIL;
		if (fs_createEntry_(fileNode_, ".", isDir_, false, fileNode_->cluster)==FS_FAIL) 
			return FS_FAIL;
	}
	return FS_OK;
}
FS_FLAGS  fs_createDirNode(const SW8* dirname_, FSNode* node_, W8 part_=0)
{
	return fs_createNode_(dirname_, node_, true, part_);
}
FS_FLAGS  fs_createFileNode(const SW8* filename_, FSNode* node_, W8 part_=0, W1 openExisting_=false)
{
	return fs_createNode_(filename_, node_, false, part_, openExisting_);
}
// aceita um pattern na forma "ficheiro%i.ext", devolve se nao existir
// com o nome conseguido em filename_ 
FS_FLAGS  fs_createFileNodeExt(SW8 *filename_, const SW8* pattern_, FSNode* node_, W8 part_=0)
{
	for (W16 i=0; i< 256; i++)
	{
		sprintf(filename_, pattern_, i);
		FS_DEBUG_LOG("\ntesting %s", filename_);
		if (fs_createFileNode(filename_, node_, part_)==FS_OK)
			return FS_OK;
	}
	return FS_FAIL;
}

 

// ------------------------------------------------------------------------

// informa em FSNode se o ficheiro é sequencial (percorre todos os cluster e verifica se são sequenciais)
// se sim, o fseek, r/w é mais rápido

struct FS_CheckSequence
{
	W1  isSequential;
	FS_CheckSequence()
	{
		isSequential = true;
	}
	W1 run(W32 clusterIdx_, W32 clusterValue_)
	{
		//printf("%i->%i->", clusterIdx_, clusterValue_);

		if (FS_CLUSTERIDX_IS_TERMINATOR(clusterValue_)) 
		{ 
			//printf("X ");
			return true;
		}

		if ((clusterValue_)!=(clusterIdx_+1)) { isSequential=0; return true; }
		return false;
	}
} action;

FS_FLAGS  fs_checkSequence(FSNode* node_)
{
	if (fs_type==FS_FAT16)
		FS_FATIterator<W16,FS_CheckSequence>::run(action, node_->cluster);
	else
		FS_FATIterator<W32,FS_CheckSequence>::run(action, node_->cluster);

	node_->isSequential = action.isSequential;
	return FS_OK;
}
W8 fs_init()
{
	INIT_SECTION(FS)

	//printf("\blk0");
	FS_DEV_GETBLK(0, fs_buff);

	W16 bootsign = *((W16*)(fs_buff+0x01FE));

	//printf("\nbootsign: %X", bootsign);

	if (bootsign!=0xAA55) fatal(FS, FS_NO_MBR_SIGN);
	
	MBRPart *part = (MBRPart*) (fs_buff + MBR_START);
	W8 i=0;
	for (i=0; i< 4; i++, part++)
		if (part->type & 0x0C) break;

	if (i==4)  fatal(FS, FS_NO_MBR_PART);
	
	fs_detectVolume_(i, true);

	//printf("\nfsOK");
	return FS_OK;
}

// assume que existe clusters livres após o cluster inicial
// fs_fastWriteBegin: começa do tamanho já existente do ficheiro, preenchendo
//                    com zeros até prefazer um tamanho ajustado ao 
//                    tamanho do cluster; devolve beginCluster_
//                    que deve ser guardado e copiado 
//						* se for greddy não verifica se há espaço à frente
//						  e assume que o ficheiro está vazio
//      fs_fastWrite: escreve um buffer (com tamanho do cluster)
//                    na primeira vez deve ser passado 
//					  uma cópia de beginCluster_
//   fs_fastWriteEnd: escreve o tamanho total e os clusters na FAT
//                    deve ser passado beginCluster_ e o último IDX dado
//					  por fs_fastWrite

FS_FLAGS  fs_fastWriteBegin(FSNode* node_, W32& beginCluster_, 
							W32& currCluster_, W32& lastCluster_, 
							W32& remain_, W1 greedy_=true)
{
	remain_ = 0;
	FS_FLAGS status = FS_OK;
	if (!node_->isValidFile()) return FS_FAIL;
	fs_checkNode_(node_);


	if (!greedy_)
	{
		// quantos clusters é o tamanho atual
		W32 clusters = fs_bytesToClusters(node_->file.fileSize);
		W32 clusterByteSize = fs_getClusterSizeInBytes();
		remain_ = clusters*clusterByteSize -  node_->file.fileSize;
		W32 bytesWr;
		FS_DEBUG_LOG("\nWriting bytes %u to ajust to aloc unit", remain_)
		if (FS_OK!=fs_write(node_, 0, remain_, bytesWr))
			return FS_FAIL;
		if (remain_!=bytesWr) return FS_FAIL;

		beginCluster_ = fs_getLastClusterIdx_(node_->cluster)+1;
		currCluster_  = beginCluster_;
		FS_DEBUG_LOG("\nCouting next free aloc blocks..",0)
	
		struct FS_CountFreeNext
		{
			W32 lastFree;
			FS_CountFreeNext()
			{
				lastFree = 0;
			}
			W1 run(W32 clusterIdx_, W32 clusterValue_)
			{
				if (clusterValue_!=0) { return true; }
				lastFree=clusterIdx_; 
				return false;
			}
		} action;

		if (fs_type==FS_FAT16)
			FS_FATIterator<W16,FS_CountFreeNext>::run(action, beginCluster_);
		else
			FS_FATIterator<W32,FS_CountFreeNext>::run(action, beginCluster_);

		if (beginCluster_ == action.lastFree) return FS_FAIL;
		lastCluster_= action.lastFree;
	}
	else
	{
		beginCluster_ = node_->cluster;
		currCluster_ = beginCluster_;
		lastCluster_= beginCluster_;
	}

	fs_setFatClusterValue_(node_->cluster, beginCluster_);
	fs_setFatClusterValue_(beginCluster_, fs_endClusterIdx);


	return FS_OK;
}
FS_FLAGS  fs_fastWrite(W0* buffer_, W32& cluster_, W32 lastCluster_, W1 stopAtLast_=false)
{
	if (stopAtLast_) { if (cluster_==lastCluster_) return FS_FAIL; }
	W8* b =  (W8*)buffer_;
	W32 LBA = fs_getClusterLBA_(cluster_) ;
	W8 lbas = fs_cluster_LBASize;
	while(lbas)
	{
		FS_SAVEBLK(LBA, b);
		b+=512;
		lbas--;
		LBA++;
	}
	cluster_++;
	return FS_OK;
}
FS_FLAGS  fs_fastWriteEnd(FSNode* node_, W32 beginCluster_, W32 endCluster_)
{
	FS_DEBUG_LOG("\nclose fast-write link cl %u to %u",beginCluster_, endCluster_);
	//atualizar a FAT, escrever todos os clusters no intervalo [begin:end]
	fs_linkClusterRange_(beginCluster_, endCluster_);
	//atualizar o tamanho na entrada de diretório
	const W32 clusterByteSize = fs_getClusterSizeInBytes();
	node_->file.fileSize =  node_->file.fileSize + 
							(endCluster_-beginCluster_)*
							clusterByteSize;
	
	FS_DEBUG_LOG("\ntotal size:%u, update entry...", node_->file.fileSize);
	node_->file.currCluster			= endCluster_;
	node_->file.absoluteOffset		= node_->file.fileSize;
	node_->file.currClusterPart		= 0;
	node_->file.currRelativeOffset	= 0;

	if ( fs_updateNodeEntry_(node_) != FS_OK)
		return FS_FAIL_TO_UPDATE;

	FS_DEBUG_LOG("fast-write OK",0);

	return FS_OK;
}





W1		  fs_getFilename(const SW8* path_, SW8* out_)
{
	W8 cnt=0;
	SW8* p = (SW8*)path_ + strlen(path_);
	do
	{
		if ((*p=='\\') || (*p=='/')) { p++; break;  }
		if (p==path_) break;
		p--;
		cnt++;
	}while(1);

	if (cnt==0)  { out_[0]=0; return FS_FAIL; }
	
	memcpy(out_, p, cnt);
	out_[cnt]=0;
	return FS_OK;
}
FS_FLAGS  fs_renameFile(const SW8* path_, SW8* newName_, W8 part_=0)
{
	SW8 filename[8+1+3+1];
	FSNode node;
	FS_FLAGS fres = fs_createFileNode(path_,&node,part_,true);
	if (fres!=FS_OK) 
	{
		FS_DEBUG_LOG("file not found",0)
		return fres;
	}
	//mudar o nome do ficheiro na entrada no diretorio
	//*** localizar o cluster com o diretorio
	if (fs_getFilename(path_, filename)!=FS_OK) return FS_FAIL;
	if (fs_updateNodeEntry_(&node, 0, true)!=FS_OK) return FS_FAIL;
	return FS_OK;
}

FS_FLAGS  fs_delete(const SW8* path_, W8 part_=0)
{
	SW8 filename[8+1+3+1];
	FSNode node;
	FS_DEBUG_LOG("\n\n---",0)
	FS_FLAGS fres = fs_createFileNode(path_,&node,part_,true);
	if (fres!=FS_OK) 
	{
		FS_DEBUG_LOG("\ndel: not found",0)
		return fres;
	}
	//*** 1 apagar no diretorio a entrada
	FS_DEBUG_LOG("\nupd node",0)
	W32 lba; W16 offset;
	if (fs_updateNodeEntry_(&node, 0, true)!=FS_OK) 
	{
		FS_DEBUG_LOG("\ndel:cant upd",0)
		return FS_FAIL;
	}
	//*** 2 apagar os clusters na FAT 
	
	W32 currCluster = node.cluster, nextCluster;
	fs_invalidate_fat_cache_();
	FS_DEBUG_LOG("\ndel: erase ",0)
	#ifdef FS_DEBUG_DELETE
		W32 cnt=1;			
	#endif
	while(1)
	{
		FS_DEBUG_LOG("%08Xh > ",currCluster)
		nextCluster = currCluster;
		const W1 isEnd = fs_getNextClusterIdx_(nextCluster)==FS_FAIL;
		fs_setFatClusterValue_(currCluster, 0);
		currCluster = nextCluster;
		if (isEnd) 
		{
			FS_DEBUG_LOG("end ",0)
			break;
		}
		#ifdef FS_DEBUG_DELETE
			cnt++;
		#endif
	}
	#ifdef FS_DEBUG_DELETE
		FS_DEBUG_LOG(" %i clusters deleted  ",cnt)
	#endif

	return FS_OK;
}


FS_FLAGS  fs_close(FSNode* node_)
{
	if (!node_->isValidFile()) return FS_FAIL;
	fs_checkNode_(node_);

	if ( fs_updateNodeEntry_(node_) != FS_OK)
		return FS_FAIL_TO_UPDATE;

	
	//#ifdef FS_DEBUG_UPDATE_NODE_ENTRY
		FSNode pathNode;
		fs_getRootDirNode(&pathNode);
		fs_iterateDir(&pathNode, fs_itfx_prnEntry, 0);
	//#endif


	return FS_OK;
}

//typedef (W0)(*FS_FASTFORMAT_CALLBACK)(W32,W32);
typedef W0(*FS_FASTFORMAT_CALLBACK)(W32,W32);

FS_FLAGS  fs_fastformat(W8 part_=0, 
						FS_FASTFORMAT_CALLBACK doneCallback_=
						(FS_FASTFORMAT_CALLBACK)0)
{
	#ifdef FS_DEBUG_FASTFORMAT
		printf("\nfmt: st");
	#endif

	// *** 1. verificar particao
	FS_FLAGS status = fs_detectVolume_(part_, true, 0);
	if (status!=FS_OK) return status;
	
	#ifdef FS_DEBUG_FASTFORMAT
		printf("\nfmt: rootClusIdx:%i LBASz:%i ClusterLBASz:%i", fs_rootDir_cluster, fs_rootDir_LBASize, fs_cluster_LBASize);
	#endif

	// *** 2. escrever a FAT a zeros (espaço livre)
	W8* fat = (W8*)FS_FAT_BUFFER;
	memset(fat, 0, FS_FAT_CACHE_SZ);
	W16 numCaches = fs_get_fat_total_segments_();
	#ifdef FS_DEBUG_FASTFORMAT
		printf("\nfmt: tam FAT %ib", numCaches*FS_FAT_CACHE_SZ);
	#endif

	for (W16 i=0; i< numCaches; i++)
	{
		fs_currFatNum = i;
		const W32 lbaStart = fs_currFatNum* FS_FAT_CACHE_LBAS;
		#ifdef FS_DEBUG_FASTFORMAT
			printf("\nfmt: sav cache %i/%i FAT&Bkup LBAs:%i,%i",
				i, numCaches, 
				fs_fat_LBAStart + lbaStart, 
				fs_fatBackup_LBAStart + lbaStart );
		#endif
		fs_save_fat_cache_();
		if (doneCallback_) doneCallback_(i, numCaches);
	}
	
	
	// senao é uma ROOT directory com cluster tem-se de
	// colocar na FAT
	W16 numClusters;
	W1 isOlderRootDir = fs_rootDir_cluster==0;
	if (isOlderRootDir)
	{
		numClusters = FS_NUMCLUSTERS_FOR_ROOTDIR;
	}
	else
	{
		numClusters = 5;
		const W16 rootDirEndCluster =  fs_rootDir_cluster + numClusters-1;
		#ifdef FS_DEBUG_FASTFORMAT
		printf("\nfmt: link rootdir %i->%i (#%i)", fs_rootDir_cluster, rootDirEndCluster, FS_NUMCLUSTERS_FOR_ROOTDIR);
		#endif
		fs_linkClusterRange_(fs_rootDir_cluster, rootDirEndCluster);
	}
		
	// zerar as entradas de diretorio do ROOT DIR
	#ifdef FS_DEBUG_FASTFORMAT
		printf("\nfmt: zeroing rootdir");
	#endif
	W32 LBA;
	const W16 totalLBAs = numClusters * fs_cluster_LBASize;
	if (isOlderRootDir)
		LBA = fs_fat_LBAStart + fs_fat_LBASize*2;
	else
		LBA = fs_getClusterLBA_(fs_rootDir_cluster);

	memset(fs_buff, 0, 512); 
	for (W16 i=0; i< totalLBAs; i++, LBA++)
	{
		#ifdef FS_DEBUG_FASTFORMAT
			printf("\nfmt: zero rootdir %i/%i LBA %i", i, totalLBAs, LBA);
		#endif
		// iterar tantos LBAs quanto tenha um cluster
		FS_SAVEBLK(LBA, fs_buff); 
	}

	#ifdef FS_DEBUG_FASTFORMAT
		printf("\nfmt: done");
	#endif
	if (doneCallback_) doneCallback_(100, 100);
	return FS_OK;
}

