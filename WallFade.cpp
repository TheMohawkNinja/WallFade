//Dependancies: ImageMagick, OSC Escape Sequence Support

#include <cstdlib>
#include <stdarg.h>
#include <Magick++.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <string>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <errno.h>
#include <thread>
#include <sys/time.h>
//#include "setroot.h"

using namespace std;
using namespace Magick;

ifstream readstream;
ofstream writestream;
struct timeval currentTime;
int confIndex_int, ctr, Rctr, Gctr, Bctr, bgW, bgH, rndHold, rndHoldOld, monitors, maxPics;
int milisecond=1000;
int second=1000*milisecond;
int threshold=100;
int steps=25;
int delay=60*second;
int subdelay=0;
const int maxMonitors=8;
const int maxTerms=100;
int monitorXRes[maxMonitors],  monitorYRes[maxMonitors],  monitorXOff[maxMonitors],  monitorYOff[maxMonitors];
int term[maxTerms], termX[maxTerms], termY[maxTerms];
double avgR, avgG, avgB, avgRTrans, avgGTrans, avgBTrans, r, g, b, fadePoint;
bool colorForce=false;
bool fadeForeground=false;
bool silent=true;
bool busyPrinting[maxTerms];
string streambuffer;
string color;
string line;
string avgRHex[maxMonitors];
string avgGHex[maxMonitors];
string avgBHex[maxMonitors];
string newpic;
string oldpic;
string resolution;
string* pics;
string termID[maxTerms];
string user=getlogin();
string configpath="/home/"+user+"/.config/WallFade/";
string picpath="/home/"+user+"/Pictures/";
string::size_type confIndex;
stringstream stream;
ColorRGB AVG, oldAVG[maxMonitors];
ColorRGB ColorSample[30][30], oldColorSample[30][30];
Image background, lastbackground;
Image A, B, maskA, maskB, compResult;
Display* dpy=XOpenDisplay(NULL);
Screen* screen=DefaultScreenOfDisplay(dpy);
XRRScreenResources* XRRscreen;
XRRCrtcInfo *crtc_info;

void output(FILE* stream, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	if(!silent)
	{
		vfprintf(stream,format,args);
	}

	va_end(args);
}
bool fexists(const char *filename)
{
	ifstream ifile(filename);
	return (bool)ifile;
}
bool to_bool(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	std::istringstream is(str);
	bool b;
	is >> std::boolalpha >> b;
	return b;
}
void calcAvg(int c)
{
	double thresholdBoostRatio=((double)threshold/255);
	output(stdout,"Total: %d, %d, %d\n",(int)avgR,(int)avgG,(int)avgB);
	output(stdout,"Divider: %d\n",c);
	output(stdout,"Threshold: %d\n",threshold);
	avgR/=(int)(c*255);
        avgG/=(int)(c*255);
        avgB/=(int)(c*255);
        
	//Boost color to threshold if all channels are below threshold
	if((int)(avgR*255)<threshold&&(int)(avgG*255)<threshold&&(int)(avgB*255)<threshold)
	{
		if(avgR>avgG&&avgR>avgB)
		{
			output(stdout,"Boosting on red by adding (%f-%d) to all color channels\n",thresholdBoostRatio,avgR);
			avgR+=thresholdBoostRatio-avgR;
			avgG+=thresholdBoostRatio-avgR;
			avgB+=thresholdBoostRatio-avgR;
		}
		else if(avgG>avgR&&avgG>avgB)
		{
			output(stdout,"Boosting on green by adding (%f-%d) to all color channels\n",thresholdBoostRatio,avgG);
			avgR+=thresholdBoostRatio-avgG;
			avgG+=thresholdBoostRatio-avgG;
			avgB+=thresholdBoostRatio-avgG;
	
		}
		else
		{
			output(stdout,"Boosting on blue by adding (%f-%d) to all color channels\n",thresholdBoostRatio,avgB);
			avgR+=thresholdBoostRatio-avgB;
			avgG+=thresholdBoostRatio-avgB;
			avgB+=thresholdBoostRatio-avgB;
		}
	}
	output(stdout,"Pre-multiply result: %f, %f, %f\n",avgR,avgG,avgB);
	output(stdout,"Average (RGB): (%d,%d,%d)\n",(int)(avgR*255),(int)(avgG*255),(int)(avgB*255));

        AVG=Magick::ColorRGB(avgR,avgG,avgB);
}
void averageColors(char color)
{
	int avgCtr=0;
	avgR=0;
	avgG=0;
	avgB=0;

	if(color=='r')
	{
		for(int y=0; y<30; y++)
        	{
                	for(int x=0; x<30; x++)
                	{
				if((int)(ColorSample[x][y].red()*255)>threshold)
				{
					avgR+=ColorSample[x][y].red()*255;
					avgG+=ColorSample[x][y].green()*255;
					avgB+=ColorSample[x][y].blue()*255;
					avgCtr++;
				}
			}
		}

		calcAvg(avgCtr);
	}
	else if(color=='g')
        {
                for(int y=0; y<30; y++)
                {
                        for(int x=0; x<30; x++)
                        {
                                if((int)(ColorSample[x][y].green()*255)>threshold)
                                {
                                        avgR+=ColorSample[x][y].red()*255;
                                        avgG+=ColorSample[x][y].green()*255;
                                        avgB+=ColorSample[x][y].blue()*255;
					avgCtr++;
                                }
                        }
                }

		calcAvg(avgCtr);
        }
	else if(color=='b')
        {
                for(int y=0; y<30; y++)
                {
                        for(int x=0; x<30; x++)
                        {
                                if((int)(ColorSample[x][y].blue()*255)>threshold)
                                {
                                        avgR+=ColorSample[x][y].red()*255;
                                        avgG+=ColorSample[x][y].green()*255;
                                        avgB+=ColorSample[x][y].blue()*255;
					avgCtr++;
                                }
                        }
                }

		calcAvg(avgCtr);
        }
}
void updateTermColors()
{
	//Refresh TermColors
	remove((configpath+"TermColors").c_str());

	for(int m=0; m<monitors; m++)
	{
        	system(("echo "+to_string(monitorXRes[m])+" >> "+configpath+"TermColors").c_str());
                system(("echo "+to_string(monitorYRes[m])+" >> "+configpath+"TermColors").c_str());
                system(("echo "+to_string(monitorXOff[m])+" >> "+configpath+"TermColors").c_str());
                system(("echo "+to_string(monitorYOff[m])+" >> "+configpath+"TermColors").c_str());
                system(("echo "+avgRHex[m]+avgGHex[m]+avgBHex[m]+" >> "+configpath+"TermColors").c_str());
        }
}
void foregroundColorSet(int s)
{
	//Make 30*30 sample space from image
	for(int y=0; y<30; y++)
	{
		for(int x=0; x<30; x++)
        	{
			ColorSample[x][y]=background.pixelColor(((bgW/30)*x),((bgH/30)*y));
			r=ColorSample[x][y].red()*255;
			g=ColorSample[x][y].green()*255;
			b=ColorSample[x][y].blue()*255;

			if(r>threshold)
			{
				Rctr++;
				avgR+=r;
				avgG+=g;
				avgB+=b;
				ctr++;
       			}
			else if(g>threshold)
			{
				Gctr++;
				avgR+=r;
                                avgG+=g;
                                avgB+=b;
                                ctr++;
			}
			else if(b>threshold)
			{
				Bctr++;
				avgR+=r;
                                avgG+=g;
                                avgB+=b;
                                ctr++;
			}
		}
	}

	if(!colorForce)
	{
		//Get average of saturated colors
		calcAvg(ctr);
	}
	else
	{
		if(Rctr>Gctr&&Rctr>Bctr)//Red
		{
			output(stdout,"Major color: Red (%d)\n",Rctr);
			averageColors('r');
		}
		else if(Gctr>Rctr&&Gctr>Bctr)//Green
                {
			output(stdout,"Major color: Green (%d)\n",Gctr);
                        averageColors('g');
                }
		else if(Gctr>Rctr&&Gctr>Bctr)//Blue
                {
			output(stdout,"Major color: Blue (%d)\n",Bctr);
                        averageColors('b');
                }
		else
		{
			output(stdout,"Major color: Other\n");
			calcAvg(ctr);
		}
	}

	Rctr=0;
	Gctr=0;
	Bctr=0;

	//Convert averages to hex
	stream<<std::hex<<(int)(avgR*255);
	avgRHex[s]=stream.str();
	stream.str("");
	stream<<std::hex<<(int)(avgG*255);
        avgGHex[s]=stream.str();
	stream.str("");
	stream<<std::hex<<(int)(avgB*255);
        avgBHex[s]=stream.str();
	stream.str("");
	output(stdout,"Average (hex): #%s%s%s\n",avgRHex[s].c_str(),avgGHex[s].c_str(),avgBHex[s].c_str());

	//Save against all black text for troubleshooting
        if(avgGHex[s]=="0"&&avgGHex[s]=="0"&&avgBHex[s]=="0")
        {
		output(stderr,"Black Save!\n");
                avgRHex[s]="ff";
                avgGHex[s]="ff";
                avgBHex[s]="ff";
        }
	output(stdout,"\n");
}
void foregroundColorPrint(string R, string G, string B, int t)
{
	busyPrinting[t]=true;
	system(("printf \"\033]10;#"+R+G+B+"\007\" > /dev/pts/"+to_string(t)).c_str());
	busyPrinting[t]=false;
}
void foregroundColorApply(int m)
{
	//Set currently open terminals to the new color
	output(stdout,"Calling OSC escape sequences to change terminal colors on monitor: %d\n",m);
	for(int i=0; i<maxTerms; i++)
	{
		if(term[i]!=0)
		{
			output(stdout,"Terminal %d check...\n",term[i]);
			if(termX[i]>monitorXOff[m]&&termX[i]<(monitorXOff[m]+monitorXRes[m]))
			{
				output(stdout,"PASS: %d>%d && %d<%d\n",termX[i],monitorXOff[m],termX[i],(monitorXOff[m]+monitorXRes[m]));
				if(termY[i]>monitorYOff[m]&&termY[i]<(monitorYOff[m]+monitorYRes[m]))
	                       	{
					output(stdout,"PASS: %d>%d && %d<%d\n",termY[i],monitorYOff[m],termY[i],(monitorYOff[m]+monitorYRes[m]));
					output(stdout,"\t on %d\n",i);
					if(!busyPrinting[term[i]])
					{
						std::thread ApplyColor(foregroundColorPrint,avgRHex[m],avgGHex[m],avgBHex[m],term[i]);
						ApplyColor.detach();
					}
      		 			//system(("printf \"\033]10;#"+avgRHex[m]+avgGHex[m]+avgBHex[m]+"\007\" > /dev/pts/"+to_string(term[i])).c_str());
       	       			}
				else
				{
					output(stdout,"FAIL: %d<=%d || %d>=%d\n",termY[i],monitorYOff[m],termY[i],(monitorYOff[m]+monitorYRes[m]));
				}
			}
			else
               	 	{
				output(stdout,"FAIL %d<=%d || %d>=%d\n",termX[i],monitorXOff[m],termX[i],(monitorXOff[m]+monitorXRes[m]));
               	 	}
		}
	}
	updateTermColors();
}
void readConfig()
{
	readstream.open((configpath+"config").c_str());

	for(int i=0; getline(readstream,line); i++)
        {
		if(!(line.find("#") != std::string::npos))
		{
                	if(line.find("path") != std::string::npos)
                	{
				confIndex=line.find("=",0);
				picpath=line.substr(confIndex+1,(line.length()-confIndex));
                        	output(stdout,"Setting image path to: %s\n",picpath.c_str());
                	}

			if(line.find("steps") != std::string::npos)
                	{
                	        confIndex=line.find("=",0);

				//Convert string to int
				std::istringstream s ((line.substr(confIndex+1,(line.length()-confIndex))));
				s >> confIndex_int;

				//If the user's step count is evenly divisible from 100, use it
				if(100%confIndex_int==0)
				{
        	                	steps=confIndex_int;
					output(stdout,"Setting steps to: %d\n",steps);
				}
				else
				{
					output(stderr,"\"%s\" does not evenly divide into 100, using default step count\n",(line.substr(confIndex,(line.length()-confIndex))).c_str());
				}
                	}

			if(line.find("delay") != std::string::npos && !(line.find("sub") != std::string::npos))
                	{
                        	confIndex=line.find("=",0);

				//Convert string to int
                        	std::istringstream s ((line.substr(confIndex+1,(line.length()-confIndex))));
                       	 	s >> delay;

				output(stdout,"Setting delay to %d seconds\n",delay);
				delay*=second;
               		}

			if(line.find("subdelay") != std::string::npos)
        	        {
	                        confIndex=line.find("=",0);

                        	//Convert string to int
                        	std::istringstream s ((line.substr(confIndex+1,(line.length()-confIndex))));
                        	s >> subdelay;

				output(stdout,"Setting subdelay to %d miliseconds\n",delay);
                        	subdelay*=milisecond;
                	}

			if(line.find("threshold") != std::string::npos)
                        {
                                confIndex=line.find("=",0);

                                //Convert string to int
                                std::istringstream s ((line.substr(confIndex+1,(line.length()-confIndex))));
                                s >> threshold;

				output(stdout,"Setting threshold to %d\n",threshold);
                        }

			if(line.find("colorForce") != std::string::npos)
                        {
                                colorForce=to_bool(line.substr(line.find("=",0)+1,(line.length()-(line.find("=",0)+1))));

				output(stdout,"Setting colorForce to %s\n",colorForce ? "true" : "false");
                        }

			if(line.find("fadeForeground") != std::string::npos)
                        {
                                fadeForeground=to_bool(line.substr(line.find("=",0)+1,(line.length()-(line.find("=",0)+1))));

				output(stdout,"Setting fadeForeground to %s\n",fadeForeground ? "true" : "false");
                        }

			if(line.find("silent") != std::string::npos)
                        {
                                silent=to_bool(line.substr(line.find("=",0)+1,(line.length()-(line.find("=",0)+1))));

				output(stdout,"Setting silent to %s\n",silent ? "true" : "false");
                        }
		}
        }
        readstream.close();
}
void makeConfig()
{
	output(stderr,"%s not found, creating...\n",(configpath+"config").c_str());
	writestream.open((configpath+"config").c_str());
	writestream<<"#Path where your wallpapers are located. Default: "+picpath+"\n";
	writestream<<"path="+picpath+"\n\n";
	writestream<<"#Number of transitions steps for fade sequence. More steps means more cached images, but a smoother transition. Default: "+to_string(steps)+"\n";
	writestream<<"steps="+to_string(steps)+"\n\n";
	writestream<<"#Number of seconds between wallpaper change. Default: "+to_string(delay/second)+"\n";
	writestream<<"delay="+to_string(delay/second)+"\n\n";
	writestream<<"#Number of miliseconds between each transition step. Will make fade last longer, but fade will look choppy if this value is too high. Default: "+to_string(subdelay/milisecond)+"\n";
	writestream<<"subdelay="+to_string(subdelay/milisecond)+"\n\n";
	writestream<<"#The threshold for each 8-bit color channel that the system will use to determine which pixels will be selected for determining terminal color. Default: "+to_string(threshold)+"\n";
        writestream<<"threshold="+to_string(threshold)+"\n";
	writestream<<"#If true, will attempt to force a saturated foreground color. Meant to keep colorful images from producing grey text. Default: "+to_string(colorForce)+"\n";
        writestream<<"colorForce="+to_string(colorForce)+"\n";
	writestream<<"#If true, will fade in the foreground color with the background fade in. If false, the foreground color will instantly change to the new color once the fade in has completed. Default: "+to_string(fadeForeground)+"\n";
        writestream<<"fadeForeground="+to_string(fadeForeground)+"\n";
	writestream<<"#If true, no output will be generated (so you don't have to deal with giant nohup.out files otherwise). Default: "+to_string(silent)+"\n";
	writestream<<"silent="+to_string(silent)+"\n";
	writestream.close();
}
string invertHex(string input)
{
	int octet=stoi(input,nullptr,16);
	string result;
	string preresult;

	octet=255-octet;

	std::stringstream s;
	
	s<<std::hex<<octet;
	preresult=s.str();
	if(preresult.length()==1){result+="0";}
	result+=s.str();
	s.str("");

	return result;
}
int main(int argc, char **argv)
{
	InitializeMagick(*argv);

	XRRscreen = XRRGetScreenResources (dpy, DefaultRootWindow(dpy));

	for(int i=0; i<maxTerms; i++)
	{
		busyPrinting[i]=false;
	}

	//Get number of monitors
        monitors=XRRscreen->ncrtc;

	//Get monitor info
	for(int i = 0; i < monitors; i++)
	{
		crtc_info = XRRGetCrtcInfo (dpy, XRRscreen, XRRscreen->crtcs[i]);
		if(crtc_info->width||crtc_info->height||crtc_info->x||crtc_info->y)
		{
			monitorXRes[i]=crtc_info->width;
			monitorYRes[i]=crtc_info->height;
			monitorXOff[i]=crtc_info->x;
			monitorYOff[i]=crtc_info->y;

			output(stdout,"\nMonitor %d\n",i);
			output(stdout,"\tWidth:    %d\n",monitorXRes[i]);
			output(stdout,"\tHeight:   %d\n",monitorYRes[i]);
			output(stdout,"\tX Offset: %d\n",monitorXOff[i]);
			output(stdout,"\tY Offset: %d\n",monitorYOff[i]);
		}
	}

	//Check for config file and make one if it does not exist
	if(!fexists((configpath).c_str()))
	{
		output(stdout,"%s not found, creating...\n",configpath.c_str());
		system(("mkdir "+configpath).c_str());

		if(!fexists((configpath+"config").c_str()))
        	{
			makeConfig();
			readConfig();
		}
		else
		{
			readConfig();
		}

	}
	if(!fexists((configpath+"config").c_str()))
        {
		makeConfig();
		readConfig();
        }
        else
        {
		readConfig();
        }

	//Make .pics file if it does not exist											
	if(!fexists((picpath+".cache/.pics").c_str()))
	{
		system(("mkdir "+picpath+".cache"+" && touch "+picpath+".cache/.pics").c_str());
	}

	//Make list of pics
	system(("cd "+picpath+" && ls | grep -E 'jpg|png' | grep -Ev 'transition|resize' > .cache/.pics").c_str());

	//Delete old TermInfo file and start updater program
	output(stdout,"Starting updateTermInfo\n");
	system("nohup updateTermInfo &");

	while(true)
	{
		for(int M=0; M<monitors; M++)
		{
			if(monitorXRes[M]==0){break;}

			//Determine which terminal is on what monitor
			output(stdout,"Getting terminal positions\n");
			readstream.open(configpath+"TermInfo");
			for(int i=0; getline(readstream,line); i++)
			{
				std::istringstream st (line);
				st >> term[i];

				getline(readstream,line);
				std::istringstream sid (line);
				sid >> termID[i];

				getline(readstream,line);
				std::istringstream sx (line);
				sx >> termX[i];

				getline(readstream,line);
				std::istringstream sy (line);
				sy >> termY[i];

				output(stdout,"\t/dev/pts/%d (%s): %d, %d\n",term[i],termID[i].c_str(),termX[i],termY[i]);
			}
			readstream.close();

			output(stdout,"\nWorking on monitor: %d\n",M);
			output(stdout,"Loading list of pics from: %s.cache/.pics\n",picpath.c_str());

			maxPics=0;

			//Get pic names															
			readstream.open(picpath+".cache/.pics");
			if(readstream.fail())
			{
				output(stderr,"ERROR: Unable to open \'%s.cache/.pics\'. Error code: %d\n",picpath.c_str(),errno);
			}
			else
			{
				for(int i=0; getline(readstream,line); i++)
				{
					maxPics++;
				}
				pics=new std::string[maxPics];
				readstream.close();
			}

			readstream.open(picpath+".cache/.pics");
			if(readstream.fail())
			{
				output(stderr,"ERROR: Unable to open \'%s.cache/.pics\'. Error code: %d\n",picpath.c_str(),errno);
			}
			else
			{
				for(int i=0; getline(readstream,line); i++)
				{
					pics[i]=line;
					output(stdout,"\tPic name %d: %s\n",i,pics[i].c_str());				
				}
				readstream.close();
			}

			output(stdout,"\n");

			//Get monitor resolution
			if(oldpic=="")
			{
				resolution=to_string(monitorXRes[M])+"x"+to_string(monitorYRes[M])+"!";
				output(stdout,"Desktop resolution: %s\n",resolution.c_str());

				//Get random image to start with
				srand(time(NULL));
				rndHold=rand()%maxPics;
				rndHoldOld=rndHold;
				oldpic=picpath+pics[rndHold];
				background=Image(oldpic);
				output(stdout,"Using: %s\n",pics[rndHold].c_str());

				//Scale to desktop resolution
				for(int i=0; i<monitors; i++)
				{
					if(monitorXRes[i]==0){break;}

					output(stdout,"\tImage size (original): %dx%d\n",background.columns(),background.rows());
					background.resize(resolution);
					background.write(picpath+".cache/resizeOld"+to_string(i)+".jpg");
					lastbackground=background;
					oldpic=picpath+".cache/resizeOld"+to_string(i)+".jpg";
					bgW=background.columns();
					bgH=background.rows();
					output(stdout,"\tImage size (resized): %dx%d\n\n",bgW,bgH);

					//Display image
					output(stdout,"Setting monitor %d\n",i);
					system(("nitrogen --head="+to_string(i)+" --set-scaled "+picpath+".cache/resizeOld"+to_string(i)+".jpg").c_str());

					foregroundColorSet(i);
					foregroundColorApply(i);
					oldAVG[i]=AVG;
				}
			}
			else
			{
				resolution=to_string(monitorXRes[M])+"x"+to_string(monitorYRes[M])+"!";
				output(stdout,"Desktop resolution: %s\n",resolution.c_str());

				//Get random new image
				srand(time(NULL));

				do
				{
					rndHold=rand()%maxPics;
				}while(rndHold==rndHoldOld);

				rndHoldOld=rndHold;
				newpic=picpath+pics[rndHold];
				background=Image(newpic);
				output(stdout,"Using: %s\n",pics[rndHold].c_str());

				//Scale to desktop resolution
				output(stdout,"\tImage size (original): %dx%d\n",background.columns(),background.rows());
				background.resize(resolution.substr(0,resolution.find("x"))+"x"+resolution.substr(resolution.find("x")+1,resolution.length()-(resolution.find("x")))+"!");
				background.write(picpath+".cache/resizeNew.jpg");
				newpic=picpath+".cache/resizeNew.jpg";
				bgW=background.columns();
				bgH=background.rows();
				output(stdout,"\tImage size (resized): %dx%d\n\n",bgW,bgH);

				//Solve for transition steps
				for(double i=0; i<=steps; i++)
				{
					output(stdout,"Compositing transition step %d... ",(int)i);

					A=Image(newpic);
					B=Image(picpath+".cache/resizeOld"+to_string(M)+".jpg");

					stream<<std::hex<<int((i+1)*(255/steps));
					streambuffer=stream.str();
					if(streambuffer.length()==1)
					{
						streambuffer="0"+streambuffer;
					}
					color=streambuffer;
					streambuffer="";
					stream.str("");

					output(stdout,"color: %s... ",color.c_str());

					maskA=Image(Geometry(bgW,bgH),Color("#"+color+color+color));
					maskB=Image(Geometry(bgW,bgH),Color("#"+invertHex(color)+invertHex(color)+invertHex(color)));

					A.composite(maskA,0,0,Magick::CopyAlphaCompositeOp);
					B.composite(maskB,0,0,Magick::CopyAlphaCompositeOp);

					compResult=A;
					compResult.composite(B,0,0,Magick::BlendCompositeOp);
					compResult.write((picpath+".cache/transition"+to_string((int)i)+".jpg").c_str());

					output(stdout,"Done!\n");

					usleep(delay/steps);
				}

				if(fadeForeground)
				{
					foregroundColorSet(M);
				}

				//Fade new image in
				output(stdout,"Fading new image in on %d\n",M);
				for (int i=0; i<=steps; i++)
				{
					system(("nitrogen --head="+to_string(M)+" --set-scaled "+picpath+".cache/transition"+to_string(i)+".jpg").c_str());

					if(fadeForeground)
					{
						//Solve for inbetween foreground colors
						fadePoint=(double)i/steps;
						output(stdout,"Fade point: %f\n",fadePoint);
						avgRTrans=(int)(((AVG.red()*255)*fadePoint)+((oldAVG[M].red()*255)*(1-fadePoint)));
						avgGTrans=(int)(((AVG.green()*255)*fadePoint)+((oldAVG[M].green()*255)*(1-fadePoint)));
						avgBTrans=(int)(((AVG.blue()*255)*fadePoint)+((oldAVG[M].blue()*255)*(1-fadePoint)));

						output(stdout,"Average (RGB): (%d,%d,%d)\n",avgRTrans,avgGTrans,avgBTrans);

						//Convert averages to hex
						stream<<std::hex<<(int)(avgRTrans);
						avgRHex[M]=stream.str();
						stream.str("");
						stream<<std::hex<<(int)(avgGTrans);
						avgGHex[M]=stream.str();
						stream.str("");
						stream<<std::hex<<(int)(avgBTrans);
						avgBHex[M]=stream.str();
						stream.str("");
						output(stdout,"Average (hex): #%s%s%s\n",avgRHex[M].c_str(),avgGHex[M].c_str(),avgBHex[M].c_str());

						//Save against all black text for troubleshooting
						if(avgGHex[M]=="0"&&avgGHex[M]=="0"&&avgBHex[M]=="0")
						{
							output(stderr,"Black Save!\n");
							avgRHex[M]="ff";
							avgGHex[M]="ff";
							avgBHex[M]="ff";
						}
						output(stdout,"\n");

						foregroundColorApply(M);
					}

					usleep(subdelay);
				}

				system(("nitrogen --head="+to_string(M)+" --set-scaled "+newpic).c_str());

				if(fadeForeground)
				{
					//Solve for inbetween foreground colors
					output(stdout,"Fade point: 1\n");
					avgRTrans=(int)(AVG.red()*255);
					avgGTrans=(int)(AVG.green()*255);
					avgBTrans=(int)(AVG.blue()*255);

					output(stdout,"Average (RGB): (%d,%d,%d)\n",avgRTrans,avgGTrans,avgBTrans);

					//Convert averages to hex
					stream<<std::hex<<(int)(avgRTrans);
					avgRHex[M]=stream.str();
					stream.str("");
					stream<<std::hex<<(int)(avgGTrans);
					avgGHex[M]=stream.str();
					stream.str("");
					stream<<std::hex<<(int)(avgBTrans);
					avgBHex[M]=stream.str();
					stream.str("");
					output(stdout,"Average (hex): #%s%s%s\n",avgRHex[M].c_str(),avgGHex[M].c_str(),avgBHex[M].c_str());

					//Save against all black text for troubleshooting
					if(avgGHex[M]=="0"&&avgGHex[M]=="0"&&avgBHex[M]=="0")
					{
						output(stderr,"Black Save!\n");
						avgRHex[M]="ff";
						avgGHex[M]="ff";
						avgBHex[M]="ff";
					}
					output(stdout,"\n");

					foregroundColorApply(M);
				}

				//Set "new" old picture
				oldpic=newpic;
				lastbackground=background;
				system(("cp "+picpath+".cache/resizeNew.jpg"+" "+picpath+".cache/resizeOld"+to_string(M)+".jpg").c_str());
			}

			if(!fadeForeground||(oldAVG[M].red()==0&&oldAVG[M].green()==0&&oldAVG[M].blue()==0))
			{
				foregroundColorSet(M);
				foregroundColorApply(M);
			}

			//Set "new" old values
			oldAVG[M]=AVG;

			output(stdout,"Done!\n\n");
		}

		//clear cache
		system(("rm -f "+picpath+".cache/transition*").c_str());
	}

	XCloseDisplay(dpy);
	return 0;
}
