
W0 prnStatus(W1 status_)
{
	if (status_==RES_OK) 
		printf("\n>>OK");
	else
		printf("\n>>FAILED");
}

W0 sd_debug()
{
	#define NBLKS 7
	W8 buf[512];
	W8 status;
	
	DELAY_MS(5000);
	printf("\nInit");

	status = sd_init();
	if (status!=RES_OK)	fatal(SD, status);
	printf("\n\n\n------------\n\n\n");

	for (W32 i=0; i< NBLKS; i++)
	{
		memset(buf, i+1, 512); 
		printf("\n\n-------Write block %i", i);
		sprintf((SW8*)buf, "[This another test 123]");
		sprintf((SW8*)buf+0x30, "[For block %i]", i);
		status = sd_wr_block(i, buf);
		prnStatus(status);
	}

	for (W32 i=0; i< NBLKS; i++)
	{
		printf("\n\n-------\n\nRead block %i", i);
		status = sd_rd_block(i, buf);
		prnStatus(status);
		prnBuff(buf);
	}
	fflush(stdout);

	while(1){}
}

