/*
 * This file containts the functions needed to handle the Admin
 * page.  This page allows a remote user to view the usage statistics
 * via a web page.
 *
 * Richard Carlson  3/10/04
 * rcarlson@internet2.edu
 */

/* local include file contains needed structures */
#include "web100srv.h"

/* Initialize the Administrator view.  Process the data in the existing log file to
 * catch up on what's happened before.
 */

int maxc2sspd=0, minc2sspd=0, maxs2cspd=0, mins2cspd=0;
int count[16], totmismatch, totbad_cable;

float recvbwd, cwndbwd, sendbwd;
char btlneck[64];
char date[32], startdate[32], mindate[32], maxdate[32];
double oo_order;
double bw2, avgrtt, timesec, loss2;
double bwin, bwout;


int calculate(int SumRTT, int CountRTT, int CongestionSignals, int PktsOut,
		int DupAcksIn, int AckPktsIn, int CurrentMSS, int SndLimTimeRwin,
		int SndLimTimeCwnd, int SndLimTimeSender, int MaxRwinRcvd, int CurrentCwnd,
		int Sndbuf, int DataBytesOut, int mismatch, int bad_cable, int c2sspd, int s2cspd,
		int c2sdata, int s2cack, int view_flag, int debug)
{

	int congestion2=0, i;
	static int totalcnt;
        double rttsec, rwin, swin, cwin, speed;
        double rwintime, cwndtime, sendtime;
        int totaltime;
	char view_string[256], *str, tmpstr[32];
	FILE *fp;

	if (view_flag == 1) {
	    fp = fopen("/tmp/view.string", "r");
	    fgets(view_string, 256, fp);

	    str = view_string;
	    sscanf(str, "%[^,]s", tmpstr);
	    maxc2sspd = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    minc2sspd = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    maxs2cspd = atoi(tmpstr);
    
	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    mins2cspd = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    totalcnt = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    totmismatch = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    totbad_cable = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[0] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[1] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[2] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[3] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[4] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[5] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[6] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[7] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[8] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[9] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[10] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[11] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[12] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[13] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[14] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    count[15] = atoi(tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    strcpy(maxdate, tmpstr);

	    str = strchr(str, ',') +1;
	    sscanf(str, "%[^,]s", tmpstr);
	    strcpy(mindate, tmpstr);

	    for (i=0; i<strlen(mindate); i++)
		if (mindate[i] == '\n')
		    break;
	    mindate[i] = '\0';

	    if (debug > 0)
		fprintf(stderr, "Updating admin_view variables: Total count = %d\n", totalcnt);

	    fclose(fp);
	}

        switch (c2sdata) {
            case -2: sprintf(btlneck, "Insufficent Data");
                     break;
            case -1: sprintf(btlneck, "System Fault");
                     break;
            case 0:  sprintf(btlneck, "Round Trip Time");
                     break;
            case 1:  sprintf(btlneck, "Dial-up modem");
                     break;
            case 2: 
                    if (((float)c2sspd/(float)s2cspd > .8)
                                && ((float)c2sspd/(float)s2cspd < 1.2) && (c2sspd > 1000))
                        sprintf(btlneck, "T1 subnet");
                    else {
                        if (s2cack == 3)
                            sprintf(btlneck, "Cable Modem");
                        else
                            sprintf(btlneck, "DSL");
                     }
                     break;
            case 3:  sprintf(btlneck, "Ethernet");
                     break;
            case 4:  sprintf(btlneck, "T3/DS-3");
                     break;
            case 5:  sprintf(btlneck, "FastEthernet");
                     break;
            case 6:  sprintf(btlneck, "OC-12");
                     break;
            case 7:  sprintf(btlneck, "Gigabit Ethernet");
                     break;
            case 8:  sprintf(btlneck, "OC-48");
                     break;
            case 9:  sprintf(btlneck, "10 Gigabit Enet");
                     break;
            case 10: sprintf(btlneck, "Retransmissions");
        }

	/* Calculate some values */
	avgrtt = (double) SumRTT/CountRTT;
	rttsec = avgrtt * .001;
	loss2 = (double)CongestionSignals/PktsOut;
	if (loss2 == 0)
    		loss2 = .000001;	/* set to 10^-6 for now */

	oo_order = (double)DupAcksIn/AckPktsIn;
	bw2 = (CurrentMSS / (rttsec * sqrt(loss2))) * 8 / 1024 / 1024;
	totaltime = SndLimTimeRwin + SndLimTimeCwnd + SndLimTimeSender;
	rwintime = (double)SndLimTimeRwin/totaltime;
	cwndtime = (double)SndLimTimeCwnd/totaltime;
	sendtime = (double)SndLimTimeSender/totaltime;
	timesec = totaltime/1000000;

	recvbwd = ((MaxRwinRcvd*8)/avgrtt)/1000;
	cwndbwd = ((CurrentCwnd*8)/avgrtt)/1000;
	sendbwd = ((Sndbuf*8)/avgrtt)/1000;

	if ((cwndtime > .02) && (mismatch == 0) && (cwndbwd < recvbwd))
    		congestion2 = 1;

	if (totalcnt == 0) {
	    minc2sspd = c2sspd;
	    mins2cspd = s2cspd;
	    maxc2sspd = c2sspd;
	    maxs2cspd = s2cspd;
	    strcpy(startdate, date);
	    strcpy(maxdate, date);
	    strcpy(mindate, date);
	}
	if (c2sspd > maxc2sspd) {
	    maxc2sspd = c2sspd;
	    strcpy(maxdate, date);
	}
	if (s2cspd > maxs2cspd) {
	    maxs2cspd = s2cspd;
	    strcpy(maxdate, date);
	}
	if (c2sspd < minc2sspd) {
	    minc2sspd = c2sspd;
	    strcpy(mindate, date);
	}
	if (s2cspd < mins2cspd) {
	    mins2cspd = s2cspd;
	    strcpy(mindate, date);
	}

	if (debug > 0) 
	    fprintf(stderr, "Initial counter Values Totalcnt = %d, Total Mismatch = %d, Total Bad Cables = %d\n",
		totalcnt, totmismatch, totbad_cable);
	totalcnt++;
	count[c2sdata+1]++;
	if (mismatch > 0)
	    totmismatch++;
	if (bad_cable == 1)
	    totbad_cable++;
	if (debug > 0) {
	    fprintf(stderr, "Updated counter values Totalcnt = %d, Total Mismatch = %d, Total Bad Cables = %d\n",
		totalcnt, totmismatch, totbad_cable);
	    fprintf(stderr, "Individual counts = [%d", count[0]);
	    for (i=1; i<16; i++)
		fprintf(stderr, ",%d", count[i]);
	    fprintf(stderr, "]\n");
	}
	return(totalcnt);
}

gen_html(int c2sspd, int s2cspd, int MinRTT, int PktsRetrans, int Timeouts, int Sndbuf,
	int MaxRwinRcvd, int CurrentCwnd, int mismatch, int bad_cable, int totalcnt, int debug)
{
	struct hostent *hp;
	unsigned addr;
	float f;
	FILE *fp;
	char view_string[256], tmpstr[256];
	char *AdminFileName;
	int i;
	struct flock lock;

	/* sprintf(tmpstr, "%s/%s", BASEDIR, LOGFILE);
	 * AdminFileName = tmpstr;
	 */
	AdminFileName = BASEDIR"/"ADMINFILE;
	fp = fopen(AdminFileName, "w");
	/* generate an HTML page for display.  */
	fprintf(fp, "<html> <title>NDT server Admin Page</title>\n");
	fprintf(fp, "<meta http-equiv=\"refresh\" content=30 url=admin.html>\n");

	/* First build current results table */
	fprintf(fp, "<table border>\n  <tr>\n ");
	fprintf(fp, "    <th colspan=8><font size=+1>NDT Usage Statistics -- Current Test Results\n");
	fprintf(fp, "    </font></tr>\n");
	fprintf(fp, "  <tr>\n    <th>Date/Time\n    <th>Total Tests\n    <th rowspan=6>\n");
	fprintf(fp, "    <th>Bottleneck Link\n    <th>Theoritical Limit\n    <th rowspan=6>\n");
	fprintf(fp, "    <th>C2S Speed\n    <th>S2C Speed\n  </tr>\n");
	fprintf(fp, "  <tr>\n    <td align=right>%s\n    <td align=right>%d\n    <td align=right>%s\n",
				date, totalcnt, btlneck);
	if (bw2 > 1)
	    fprintf(fp, "    <td align=right>%0.2f Mbps\n",  bw2);
	else
	    fprintf(fp, "    <td align=right>%0.2f Kbps\n",  bw2*1000);
	if (c2sspd > 1000)
	    fprintf(fp, "    <td align=right>%0.2f Mbps\n    <td align=right>%0.2f Mbps\n",
		(float) c2sspd/1000, (float)s2cspd/1000);
	else
	    fprintf(fp, "    <td align=right>%d Kbps\n    <td align=right>%d Kbps, ", c2sspd, s2cspd);
	fprintf(fp, "  </tr>\n  <tr>\n    <th>Packet Loss\n    <th>Average RTT\n");
	fprintf(fp, "    <th>Mininum RTT\n    <th>Retrans/sec\n    <th>Timeouts/sec\n");
	fprintf(fp, "    <th>%% Out-of-Order\n  </tr>\n  <tr>\n");
	if (loss2 == .000001) 
	   fprintf(fp, "    <td align=right>None\n    <td align=right>%0.2f msec\n    <td align=right>%d msec\n",
			avgrtt, MinRTT);
	else
	   fprintf(fp, "    <td align=right>%0.4f%%\n    <td align=right>%0.2f msec\n    <td align=right>%d msec\n",
			loss2*100, avgrtt, MinRTT);
	fprintf(fp, "    <td align=right>%0.2f\n    <td align=right>%0.2f\n    <td align=right>%0.2f\n  </tr>\n", 
		(float) PktsRetrans/timesec, (float) Timeouts/timesec, oo_order*100);
	fprintf(fp, "  <tr>\n    <th>Send Buffer\n    <th>BW*Delay\n");
	fprintf(fp, "    <th>Receive Buffer\n    <th>BW*Delay\n");
	fprintf(fp, "    <th>Congestion Window\n    <th>BW*Delay\n  </tr>\n");
	fprintf(fp, "  <tr>\n    <td align=right>%d Bytes\n    <td align=right>%0.2f Mbps\n", Sndbuf, sendbwd);
	fprintf(fp, "    <td align=right>%d Bytes\n    <td align=right>%0.2f Mbps\n", MaxRwinRcvd, recvbwd);
	fprintf(fp, "    <td align=right>%d Bytes\n    <td align=right>%0.2f Mbps\n", CurrentCwnd, cwndbwd);
	fprintf(fp, "  </tr>\n</table>\n\n");

	/* Next write out the data needed to build the graph  */
	/* for (i=0; i<16; i++)
	 *    printf("count[%d] = %d\n", i, count[i]);
	 * printf("Total = %d\n", totalcnt);
	 */

	fprintf(fp, "<applet code=Admin.class\n  width=600 height=400>\n");
	fprintf(fp, "  <PARAM NAME=\"Fault\" VALUE=\"%d\">\n", count[0]);
	fprintf(fp, "  <PARAM NAME=\"RTT\" VALUE=\"%d\">\n", count[1]);
	fprintf(fp, "  <PARAM NAME=\"Dial-up\" VALUE=\"%d\">\n", count[2]);
	fprintf(fp, "  <PARAM NAME=\"T1\" VALUE=\"%d\">\n", count[3]);
	fprintf(fp, "  <PARAM NAME=\"Enet\" VALUE=\"%d\">\n", count[4]);
	fprintf(fp, "  <PARAM NAME=\"T3\" VALUE=\"%d\">\n", count[5]);
	fprintf(fp, "  <PARAM NAME=\"FastE\" VALUE=\"%d\">\n", count[6]);
	fprintf(fp, "  <PARAM NAME=\"OC-12\" VALUE=\"%d\">\n", count[7]);
	fprintf(fp, "  <PARAM NAME=\"GigE\" VALUE=\"%d\">\n", count[8]);
	fprintf(fp, "  <PARAM NAME=\"OC-48\" VALUE=\"%d\">\n", count[9]);
	fprintf(fp, "  <PARAM NAME=\"tenGE\" VALUE=\"%d\">\n", count[10]);
	fprintf(fp, "  <PARAM NAME=\"Total\" VALUE=\"%d\">\n", totalcnt);
	fprintf(fp, "</applet>\n<br>\n");

	/* Next generate summary table */
	fprintf(fp, "<table border>\n  <tr>\n");
	fprintf(fp, "    <th>\n    <th>Date/Time\n    <th rowspan=5>\n");
	fprintf(fp, "    <th colspan=2>Throughput Summary\n    <th rowspan=5>\n");
  	fprintf(fp, "    <th colspan=2>Configuration Fault Summary\n  </tr>\n  <tr>\n");
	fprintf(fp, "    <td><b>Log Starts</b>\n    <td align=right>%s\n    <th>Client-to-Server\n", startdate);
	fprintf(fp, "    <th>Server-to-Client\n    <th>Duplex Mismatch\n");
	fprintf(fp, "    <th>Excessive Errors\n  </tr>\n");
	fprintf(fp, "  <tr>\n    <td><b>Current</b>\n    <td align=right>%s\n", date);
	if (c2sspd > 1000)
	    fprintf(fp, "    <td align=right>%0.2f Mbps\n    <td align=right>%0.2f Mbps\n",
		(float) c2sspd/1000, (float)s2cspd/1000);
	else
	    fprintf(fp, "    <td align=right>%d Kbps\n    <td align=right>%d Kbps, ", c2sspd, s2cspd);

	fprintf(fp, "    <td align=right>%s\n    <td align=right>%s\n  </tr>\n  <tr>\n",
		mismatch==1?"Yes":"No", bad_cable==1?"Yes":"No");	
	fprintf(fp, "    <td><b>Maximum</b>\n    <td align=right>%s\n", maxdate);
	if (maxc2sspd > 1000)
	    fprintf(fp, "    <td align=right>%0.2f Mbps\n    <td align=right>%0.2f Mbps\n",
		(float) maxc2sspd/1000, (float)maxs2cspd/1000);
	else
	    fprintf(fp, "    <td align=right>%d Kbps\n    <td align=right>%d Kbps, ", maxc2sspd, maxs2cspd);
	fprintf(fp, "    <td align=right>%d found\n    <td align=right>%d found\n  </tr>\n",
		totmismatch, totbad_cable);
	fprintf(fp, "  <tr>\n    <td><b>Minimum</b>\n    <td align=right>%s\n", mindate);
	if (minc2sspd > 1000)
	    fprintf(fp, "    <td align=right>%0.2f Mbps\n    <td align=right>%0.2f Mbps\n",
		(float) minc2sspd/1000, (float)mins2cspd/1000);
	else
	    fprintf(fp, "    <td align=right>%d Kbps\n    <td align=right>%d Kbps\n", minc2sspd, mins2cspd);
	fprintf(fp, "    <td>\n    <td>\n  </tr>\n</table>\n");

	/* fprintf(fp, "<br><hr noshade width=90%% size=2>\n");
	 * fprintf(fp, "Comments to <a href=mailto:RCarlson501@comcast.net>");
	 * fprintf(fp, " Rich Carlson </a> email:RCarlson501@comcast.net\n");
	 * fprintf(fp, "<br><i>Created: Automatically by admin_view program\n");
	 * fprintf(fp, "</html>\n");
	 */

	/* now pick up the rest of the descriptive text that goes along with this page.
	 * instead of burying all the text in this program, just include it from an external
	 * file.  For now, use the system command to append the text to the just created file.
	 */
	fclose(fp);
	sprintf(tmpstr, "/bin/cat %s/admin_description.html >> %s/admin.html", BASEDIR,  BASEDIR);
	system(tmpstr);

	/* Save the current variables into a file for later use.  These
	 * variables are updated by each child process at the end of every
	 * test.  This data must be shared between multiple children.  If
	 * not, then the admin view page doesn't get update after every 
	 * test.
	 */
	fp = fopen("/tmp/view.string", "w");
	lock.l_type = F_WRLCK;
	i = fcntl((int)fp, F_SETLKW, lock);
	    if (debug > 0)
		fprintf(stderr, "successfully locked '/tmp/view.string' for updating\n");
	sprintf(view_string, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%s",
		maxc2sspd, minc2sspd, maxs2cspd, mins2cspd, totalcnt,
		totmismatch, totbad_cable, count[0], count[1], count[2], count[3], count[4],
		count[5], count[6], count[7], count[8], count[9], count[10], count[11],
		count[12], count[13], count[14], count[15], maxdate, mindate);
	if (debug > 0)
		fprintf(stderr, "sending '%s' to tmp file\n", view_string);
	fprintf(fp, "%s\n", view_string);
	lock.l_type = F_UNLCK;
	fcntl((int)fp, F_SETLK, lock);
	fclose(fp);

}

view_init(char *LogFileName, int debug)
{

        double loss, rttsec, bw, rwin, swin, cwin, speed;
        double rwintime, cwndtime, sendtime;
        int experimental=0, n, m, one=1;
        int Timeouts, SumRTT, CountRTT, MinRTT, PktsRetrans, FastRetran, DataPktsOut;
        int AckPktsOut, CurrentMSS, DupAcksIn, AckPktsIn, MaxRwinRcvd, Sndbuf;
        int CurrentCwnd, SndLimTimeRwin, SndLimTimeCwnd, SndLimTimeSender, DataBytesOut;
        int SndLimTransRwin, SndLimTransCwnd, SndLimTransSender, MaxSsthresh;
        int CurrentRTO, CurrentRwinRcvd, CongestionSignals, PktsOut;
        FILE *fp;
        int c2sspd, s2cspd;
        char ip_addr2[64], buff[256], *str, tmpstr[32];
        int link=0, mismatch=0, bad_cable=0, half_duplex=0, congestion=0, totaltime;
        int c2sdata, c2sack, s2cdata, s2cack;
	int totalcnt=0, view_flag=0;

	if ((fp = fopen(LogFileName, "r")) == NULL)
	    return;

	while ((fgets(buff, 512, fp)) != NULL) {
	    if ((str = strchr(buff, ',')) != NULL) {
		sscanf(buff, "%[^,]s", date);
		str++;
		sscanf(str, "%[^,]s", ip_addr2);
		if ((str = strchr(str, ',')) == NULL)
		    continue;
		str++;
		sscanf(str, "%[^,]s", tmpstr);
		s2cspd = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		c2sspd = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		Timeouts = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		SumRTT = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		CountRTT = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		PktsRetrans = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		FastRetran = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		DataPktsOut = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		AckPktsOut = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		CurrentMSS = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		DupAcksIn = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		AckPktsIn = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		MaxRwinRcvd = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		Sndbuf = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		CurrentCwnd = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		SndLimTimeRwin = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		SndLimTimeCwnd = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		SndLimTimeSender = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		DataBytesOut = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		SndLimTransRwin = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		SndLimTransCwnd = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		SndLimTransSender = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		MaxSsthresh = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		CurrentRTO = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		CurrentRwinRcvd = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		link = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		mismatch = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		bad_cable = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		half_duplex = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		congestion = atoi(tmpstr);

		str = strchr(str, ',');
		if (str == NULL) {
		    CongestionSignals = -1;
		    goto display;
		}
		str += 1;
		sscanf(str, "%[^,]s", tmpstr);
		c2sdata = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		c2sack = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		s2cdata = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		s2cack = atoi(tmpstr);

		str = strchr(str, ',');
		if (str == NULL) {
		    CongestionSignals = -1;
		    goto display;
		}
		str += 1;
		sscanf(str, "%[^,]s", tmpstr);
		CongestionSignals = atoi(tmpstr);

		str = strchr(str, ',') +1;
		sscanf(str, "%[^,]s", tmpstr);
		PktsOut = atoi(tmpstr);

		str = strchr(str, ',');
		if (str == NULL)
		    MinRTT = -1;
		else {
		    str += 1;
		    sscanf(str, "%[^,]s", tmpstr);
		    MinRTT = atoi(tmpstr);
		}

display:
		if (debug > 3)
		    fprintf(stderr, "Web100 variables line received\n\n");
		totalcnt = calculate(SumRTT, CountRTT, CongestionSignals, PktsOut, DupAcksIn, AckPktsIn,
			CurrentMSS, SndLimTimeRwin, SndLimTimeCwnd, SndLimTimeSender,
			MaxRwinRcvd, CurrentCwnd, Sndbuf, DataBytesOut, mismatch, bad_cable,
			c2sspd, s2cspd, c2sdata, s2cack, view_flag, debug);
	    }

	}
	fclose(fp);
	view_flag = 1;
	gen_html(c2sspd, s2cspd, MinRTT, PktsRetrans, Timeouts, Sndbuf, MaxRwinRcvd,
		CurrentCwnd, mismatch, bad_cable, totalcnt, debug);

}
