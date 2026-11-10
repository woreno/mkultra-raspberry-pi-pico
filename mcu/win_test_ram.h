
W8 *ramBuf;

W1 ram_rd_block(W32 LBA_, W8* buf_)
{
	W64 ptr = ((W64)LBA_)*512;
	memcpy(buf_, ramBuf+ptr, 512); 
	return RES_OK;
}
W1 ram_wr_block(W32 LBA_, W8* buf_)
{
	W64 ptr = ((W64)LBA_)*512;
	if (buf_)
		memcpy(ramBuf+ptr, buf_, 512);
	else
		memset(ramBuf+ptr, 0, 512);

	return RES_OK;

}