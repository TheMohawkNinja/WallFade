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
#include <errno.h>
#include <thread>
#include <sys/time.h>

using namespace std;
using namespace Magick;

ifstream readstream;
ofstream writestream;
struct timeval currentTime;
int confIndex_int, ctr, Rctr, Gctr, Bctr, bgW, bgH, rndHold, rndHoldOld, screens, maxPics;
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
long int bgPaintTime;
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
Display* d=XOpenDisplay(NULL);
Screen* s=DefaultScreenOfDisplay(d);

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
string getCmdOut(string cmd)
{
	string data;
	FILE* stream;
	const int max_buffer = 256;
	char buffer[max_buffer];
	cmd.append(" 2>&1");
	stream=popen(cmd.c_str(), "r");

	if(stream)
	{
		while (!feof(stream))
		if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
		pclose(stream);
	}

	return data;
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

	for(int s=0; s<screens; s++)
	{
        	system(("echo "+to_string(monitorXRes[s])+" >> "+configpath+"TermColors").c_str());
                system(("echo "+to_string(monitorYRes[s])+" >> "+configpath+"TermColors").c_str());
                system(("echo "+to_string(monitorXOff[s])+" >> "+configpath+"TermColors").c_str());
                system(("echo "+to_string(monitorYOff[s])+" >> "+configpath+"TermColors").c_str());
                system(("echo "+avgRHex[s]+avgGHex[s]+avgBHex[s]+" >> "+configpath+"TermColors").c_str());
        }
}
void setBackground(std::string h, std::string p, std::string i)
{
	if(i!="")
	{
		system(("nitrogen --head="+h+" --set-scaled "+p+".cache/transition"+i+".jpg").c_str());
	}
	else
	{
		system(("nitrogen --head="+h+" --set-scaled "+p).c_str());
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

	for(int i=0; i<maxTerms; i++)
	{
		busyPrinting[i]=false;
	}

	//Get number of screens
	std::istringstream ss (getCmdOut("xrandr | grep -v disconnected | grep -o connected | wc -l"));
        ss >> screens;

	//Get monitor info
	output(stdout,"Monitor X resolutions\n");
	for(int i=0; i<screens; i++)
	{
		std::istringstream ss (getCmdOut("xrandr | grep -E '[d|y] [0-9]{2,4}x[0-9]{2,4}' | grep -Eo '[0-9]{2,4}x[0-9]{2,4}' | grep -Eo '[0-9]{2,4}x' | grep -Eo '[0-9]{2,4}' | sed -n '"+to_string(i+1)+" p'"));
		ss >> monitorXRes[i];
		output(stdout,"%d\n",monitorXRes[i]);
	}

	output(stdout,"Monitor Y resolutions\n");
        for(int i=0; i<screens; i++)
        {
                std::istringstream ss (getCmdOut("xrandr | grep -E '[d|y] [0-9]{2,4}x[0-9]{2,4}' | grep -Eo '[0-9]{2,4}x[0-9]{2,4}' | grep -Eo 'x[0-9]{2,4}' | grep -Eo '[0-9]{2,4}' | sed -n '"+to_string(i+1)+" p'"));
                ss >> monitorYRes[i];
		output(stdout,"%d\n",monitorYRes[i]);
        }

	output(stdout,"Monitor X offsets\n");
	for(int i=0; i<screens; i++)
	{
		std::istringstream ss (getCmdOut("xrandr | grep -E '[d|y] [0-9]{2,4}x[0-9]{2,4}' | grep -Eo '[0-9]{2,4}x[0-9]{2,4}[+][0-9]{0,4}[+][0-9]{0,4}' | grep -Eo '[+][0-9]{0,4}[+]' | grep -Eo '[0-9]{0,4}' | sed -n '"+to_string(i+1)+" p'"));
                ss >> monitorXOff[i];
		output(stdout,"%d\n",monitorXOff[i]);
	}

	output(stdout,"Monitor Y offsets\n");
        for(int i=0; i<screens; i++)
        {
                std::istringstream ss (getCmdOut("xrandr | grep -E '[d|y] [0-9]{2,4}x[0-9]{2,4}' | grep -Eo '[0-9]{2,4}x[0-9]{2,4}[+][0-9]{0,4}[+][0-9]{0,4}' | grep -Eo '[+][0-9]{0,4}( |$)' | grep -Eo '[0-9]{0,4}' | sed -n '"+to_string(i+1)+" p'"));
                ss >> monitorYOff[i];
		output(stdout,"%d\n",monitorYOff[i]);
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
		for(int S=0; S<screens; S++)
		{
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

			output(stdout,"\nWorking on screen: %d\n",S);
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
				resolution=getCmdOut("xrandr | grep connected | grep -v disconnected |  grep -Eo \"[0-9]{2,4}x[0-9]{0,4}+[0-9]{0,4}\" | sed -n '"+to_string(S+1)+" p'");
				output(stdout,"Desktop resolution: %s\n",resolution.c_str());

				//Get random image to start with
				srand(time(NULL));
				rndHold=rand()%maxPics;
				rndHoldOld=rndHold;
				oldpic=picpath+pics[rndHold];
				background=Image(oldpic);
				output(stdout,"Using: %s\n",pics[rndHold].c_str());

				//Scale to desktop resolution
				for(int i=0; i<screens; i++)
				{
					output(stdout,"\tImage size (original): %dx%d\n",background.columns(),background.rows());
					background.resize(resolution.substr(0,resolution.find("x"))+"x"+resolution.substr(resolution.find("x")+1,resolution.length()-(resolution.find("x")))+"!");
					background.write(picpath+".cache/resizeOld"+to_string(i)+".jpg");
					lastbackground=background;
					oldpic=picpath+".cache/resizeOld"+to_string(i)+".jpg";
					bgW=background.columns();
					bgH=background.rows();
					output(stdout,"\tImage size (resized): %dx%d\n\n",bgW,bgH);

					//Display image
					output(stdout,"Setting screen %d\n",i);
					system(("nitrogen  --head="+to_string(i)+" --set-scaled "+picpath+".cache/resizeOld"+to_string(i)+".jpg").c_str());

					foregroundColorSet(i);
					foregroundColorApply(i);
					oldAVG[i]=AVG;
				}
			}
			else
			{
				resolution=getCmdOut("xrandr | grep connected | grep -v disconnected |  grep -Eo \"[0-9]{2,4}x[0-9]{0,4}+[0-9]{0,4}\" | sed -n '"+to_string(S+1)+" p'");
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
					B=Image(picpath+".cache/resizeOld"+to_string(S)+".jpg");

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

					//system(("composite -blend "+to_string((100/steps)*i)+" "+newpic+" "+picpath+".cache/resizeOld"+to_string(S)+".jpg"+" "+picpath+".cache/transition"+to_string(i)+".jpg").c_str());
					output(stdout,"Done!\n");

					usleep(delay/steps);
				}

				if(fadeForeground)
				{
					foregroundColorSet(S);
				}

				//Fade new image in
				output(stdout,"Fading new image in on %d\n",S);
				for (int i=0; i<=steps; i++)
				{
					//gettimeofday(&currentTime, NULL);
					//bgPaintTime=(currentTime.tv_sec*1000)+(currentTime.tv_usec/1000);
					std::thread setBackgroundThread(setBackground, to_string(S), picpath, to_string(i));
					setBackgroundThread.detach();
					//system(("nitrogen --head="+to_string(S)+" --set-scaled "+picpath+".cache/transition"+to_string(i)+".jpg").c_str());
					//gettimeofday(&currentTime, NULL);
					//bgPaintTime=((currentTime.tv_sec*1000)+(currentTime.tv_usec/1000))-bgPaintTime;

					if(fadeForeground)
					{
						//Solve for inbetween foreground colors
						fadePoint=(double)i/steps;
						output(stdout,"Fade point: %f\n",fadePoint);
						avgRTrans=(int)(((AVG.red()*255)*fadePoint)+((oldAVG[S].red()*255)*(1-fadePoint)));
						avgGTrans=(int)(((AVG.green()*255)*fadePoint)+((oldAVG[S].green()*255)*(1-fadePoint)));
						avgBTrans=(int)(((AVG.blue()*255)*fadePoint)+((oldAVG[S].blue()*255)*(1-fadePoint)));

						output(stdout,"Average (RGB): (%d,%d,%d)\n",avgRTrans,avgGTrans,avgBTrans);

						//Convert averages to hex
						stream<<std::hex<<(int)(avgRTrans);
						avgRHex[S]=stream.str();
						stream.str("");
						stream<<std::hex<<(int)(avgGTrans);
						avgGHex[S]=stream.str();
						stream.str("");
						stream<<std::hex<<(int)(avgBTrans);
						avgBHex[S]=stream.str();
						stream.str("");
						output(stdout,"Average (hex): #%s%s%s\n",avgRHex[S].c_str(),avgGHex[S].c_str(),avgBHex[S].c_str());

						//Save against all black text for troubleshooting
						if(avgGHex[S]=="0"&&avgGHex[S]=="0"&&avgBHex[S]=="0")
						{
							output(stderr,"Black Save!\n");
							avgRHex[S]="ff";
							avgGHex[S]="ff";
							avgBHex[S]="ff";
						}
						output(stdout,"\n");

						foregroundColorApply(S);
					}

					if(subdelay-bgPaintTime>0)
					{
						usleep(subdelay-bgPaintTime);
					}
				}

				//gettimeofday(&currentTime, NULL);
				//bgPaintTime=(currentTime.tv_sec*1000)+(currentTime.tv_usec/1000);
				//system(("nitrogen --head="+to_string(S)+" --set-scaled "+newpic).c_str());
				std::thread setBackgroundThread(setBackground, to_string(S), newpic, "");
				setBackgroundThread.detach();
				//gettimeofday(&currentTime, NULL);
				//bgPaintTime=((currentTime.tv_sec*1000)+(currentTime.tv_usec/1000))-bgPaintTime;

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
					avgRHex[S]=stream.str();
					stream.str("");
					stream<<std::hex<<(int)(avgGTrans);
					avgGHex[S]=stream.str();
					stream.str("");
					stream<<std::hex<<(int)(avgBTrans);
					avgBHex[S]=stream.str();
					stream.str("");
					output(stdout,"Average (hex): #%s%s%s\n",avgRHex[S].c_str(),avgGHex[S].c_str(),avgBHex[S].c_str());

					//Save against all black text for troubleshooting
					if(avgGHex[S]=="0"&&avgGHex[S]=="0"&&avgBHex[S]=="0")
					{
						output(stderr,"Black Save!\n");
						avgRHex[S]="ff";
						avgGHex[S]="ff";
						avgBHex[S]="ff";
					}
					output(stdout,"\n");

					foregroundColorApply(S);
				}

				//Set "new" old picture
				oldpic=newpic;
				lastbackground=background;
				system(("cp "+picpath+".cache/resizeNew.jpg"+" "+picpath+".cache/resizeOld"+to_string(S)+".jpg").c_str());
			}

			if(!fadeForeground||(oldAVG[S].red()==0&&oldAVG[S].green()==0&&oldAVG[S].blue()==0))
			{
				foregroundColorSet(S);
				foregroundColorApply(S);
			}

			//Set "new" old values
			oldAVG[S]=AVG;

			output(stdout,"Done!\n\n");
		}

		//clear cache
		system(("rm -f "+picpath+".cache/transition*").c_str());
	}

	XCloseDisplay(d);
	return 0;
}
