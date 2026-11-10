#include <winioctl.h>
#include <sddl.h>
#define DEV_DRIVE_PATH "\\\\.\\PhysicalDrive3"
#define SD_INIT_FAIL(X_) {err = GetLastError(); DEBUG("\n*** " #X_ " : err:%i", err) goto fail;}
#define DEVBLK_SZ 512
enum    DEV_LAST_ACTION { DEV_LAST_ACTION_NONE, DEV_LAST_ACTION_R, DEV_LAST_ACTION_W };
DEV_LAST_ACTION sd_lastAction = DEV_LAST_ACTION_NONE;


HANDLE device = 0;
W32    dev_numBlocks;
W32    dev_heads, dev_sects_per_cil;
W1     dev_firsttime = true;

CREATE_DISK   rawDisk; 

W1 sd_close_dev()
{
    DWORD err, status;
	if (device)
	{
		
		if (!DeviceIoControl(device, FSCTL_UNLOCK_VOLUME,NULL, 0, NULL, 0, &status, NULL))
			SD_INIT_FAIL(FSCTL_LOCK_VOLUME)
		CloseHandle(device);
	}
	device=0;
	return RES_OK;
fail:
	return RES_FAIL;
}
W1 sd_open_dev()
{
    DWORD err, status, bytes;
	sd_close_dev();

	GET_LENGTH_INFORMATION lengthInfo;
	device = 
		 CreateFileA(
			DEV_DRIVE_PATH, 
			 GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, 
			NULL, 
			OPEN_EXISTING, 
			FILE_FLAG_NO_BUFFERING , // FILE_FLAG_BACKUP_SEMANTICS FILE_FLAG_NO_BUFFERING 
			0);

	if (device==INVALID_HANDLE_VALUE)
	{
		err = GetLastError(); 
		DEBUG("\n*** CreateDevice : err:%i", err)
		return RES_FAIL;
	}
	STORAGE_DEVICE_NUMBER storageDeviceNumber = { 0 };
    if (!DeviceIoControl(device, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &storageDeviceNumber, sizeof(storageDeviceNumber), &bytes, NULL))
		SD_INIT_FAIL(IOCTL_DISK_GET_LENGTH_INFO)
		

	if (dev_firsttime)
	{
		if (!DeviceIoControl(device, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &lengthInfo,
							sizeof(lengthInfo), &bytes, NULL)) 
		SD_INIT_FAIL(IOCTL_DISK_GET_LENGTH_INFO)
		dev_firsttime = false;
	} 
	
	//DWORD ret = LockFile(device, 0, 0, 3 * 1023 * 1023 * 1023, 0);

	if (!DeviceIoControl(device, FSCTL_LOCK_VOLUME,NULL, 0, NULL, 0, &status, NULL))
		SD_INIT_FAIL(FSCTL_LOCK_VOLUME)
	
	if (!DeviceIoControl(device, FSCTL_DISMOUNT_VOLUME,NULL, 0, NULL, 0, &status, NULL))
		SD_INIT_FAIL(FSCTL_DISMOUNT_VOLUME)

		
	DEBUG("\n\n\n*** Device locked/unmounted '%s' has the number %i", DEV_DRIVE_PATH, storageDeviceNumber.DeviceNumber);

	return RES_OK;
fail:
	sd_close_dev();
	return RES_FAIL;
}
W1 sd_createRaw_and_open()
{
  DWORD err,  bytes;
  if (sd_open_dev()==RES_FAIL) return RES_FAIL;

  ZeroMemory(&rawDisk,sizeof(rawDisk));
  rawDisk.PartitionStyle = PARTITION_STYLE_RAW;
  if (!DeviceIoControl(device, IOCTL_DISK_CREATE_DISK, &rawDisk, sizeof(rawDisk), NULL, 0, &bytes, NULL))
    SD_INIT_FAIL(IOCTL_DISK_CREATE_DISK)

  return sd_open_dev();
fail:
  sd_close_dev();
  return RES_FAIL;
}

W1 sd_rd_block(W32 LBA_, const W0* bf_,  W32 nBlocks=1)
{
	W8* buf_ = (W8*)bf_;
	DWORD err;
	//if (sd_lastAction!=DEV_LAST_ACTION_R)
	//	printf("\n--- dev read LBA:");
	//sd_lastAction = DEV_LAST_ACTION_R;
	//	printf(" 0x%X", LBA_);
	
	W64 bytesToWrite = 512*nBlocks;
	W64 ptr = LBA_*512;
	LONG ptrHi = ptr>>32;
	DWORD bytesWr;
	SetLastError (0);    
	if (SetFilePointer(device, ptr, &ptrHi, FILE_BEGIN)==INVALID_SET_FILE_POINTER)
		SD_INIT_FAIL(ReadFile.SetFilePointer)

	if (ReadFile(device, buf_ , bytesToWrite, &bytesWr, NULL)==0)
		SD_INIT_FAIL(ReadFile)
	
	if (bytesWr!=bytesToWrite) SD_INIT_FAIL(ReadFile.BytesWritten)

		
	return RES_OK;
fail:
	return RES_FAIL;
}
W1 sd_wr_block(W32 LBA_, const W0* bf_, W32 nBlocks=1)
{
	W8* buf_ = (W8*)bf_;
	//if (sd_lastAction!=DEV_LAST_ACTION_W)
	//	printf("\n--- dev write LBA:");
	//sd_lastAction = DEV_LAST_ACTION_W;
	//	printf(" 0x%X", LBA_);
	//
	DWORD err, bytesWr = DEVBLK_SZ;

	W64 bytesToWrite = 512*nBlocks;
	W64 ptr = ((W64)LBA_)*512;
	LONG ptrHi = ptr>>32;
	
	SetLastError (0);    
	if (SetFilePointer(device, ptr, &ptrHi, FILE_BEGIN)==INVALID_SET_FILE_POINTER)
		SD_INIT_FAIL(WriteFile.SetFilePointer)

	if (WriteFile(device, buf_ , bytesToWrite, &bytesWr, NULL)==0) 
		SD_INIT_FAIL(WriteFile)
	
	if (bytesWr!=bytesToWrite)  SD_INIT_FAIL(WriteFile.BytesWritten)
		

	return RES_OK;
fail:
	return RES_FAIL;
}
W1 sd_eraseFirstBlocks(W32 blks_=dev_numBlocks)
{
	W8 buf[512];
	memset(buf, 0, 512);
	for (SW32 i=0; i< blks_; i++)	
	{
		printf("\nApagar bloco %i/%i", i,dev_numBlocks);
		if (sd_wr_block(i, buf)==RES_FAIL)
			return RES_FAIL;
	}
	return RES_FAIL;
}
 