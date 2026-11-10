
W0 format_disk_progbar(W32 done_, W32 total_)
{
	if ((done_%32==0)||(done_==total_))
		draw_progressbar(uiProgressBar, Color_Blue,  done_, total_);
}
W0 format_disk(W0* args_)
{
	ui_display_str(ui_strs_wait, ui_strs_formatDisk);
	ui_init_progressBar();
	fs_fastformat(0, format_disk_progbar);
	ui_display_str(ui_strs_done, ui_strs_formatDisk);
}
