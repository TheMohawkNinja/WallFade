//Dependancies: ImageMagick, OSC Escape Sequence Support

#include <cstdlib>
#include <Magick++.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <string>
#include <unistd.h>
#include <X11/Xlib.h>
#include <errno.h>

using namespace std;
using namespace Magick;

ifstream readstream;
ofstream writestream;
int confIndex_int, Rctr, Gctr, Bctr, ctr, bgW, bgH, rndHold, rndHoldOld, screens;
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
string line;
string avgRHex[maxMonitors];
string avgGHex[maxMonitors];
string avgBHex[maxMonitors];
string newpic;
string oldpic;
string resolution;
string pics[100];
string termID[maxTerms];
string user=getlogin();
string configpath="/home/"+user+"/.config/WallFade/";
string picpath="/home/"+user+"/Pictures/";
string::size_type confIndex;
stringstream stream;
ColorRGB AVG, oldAVG[maxMonitors];
ColorRGB ColorSample[30][30], oldColorSample[30][30];
Image background;
Image lastbackground;
Display* d=XOpenDisplay(NULL);
Screen* s=DefaultScreenOfDisplay(d);

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
	cout<<"Total: "<<(int)avgR<<", "<<(int)avgG<<", "<<(int)avgB<<endl;
	cout<<"Divider: "<<c<<endl;
	avgR/=(int)(c*255);
        avgG/=(int)(c*255);
        avgB/=(int)(c*255);
        cout<<"Average (RGB): ("<<(int)(avgR*255)<<","<<(int)(avgG*255)<<","<<(int)(avgB*255)<<")"<<endl;
        AVG=Magick::ColorRGB(avgR,avgG,avgB);
}

void averageColors(char color)
{
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
				}
			}
		}

                calcAvg(Rctr);
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
                                }
                        }
                }

		calcAvg(Gctr);
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
                                }
                        }
                }

		calcAvg(Bctr);
        }
}

void updateTermColors()
{
	//Refresh TermColors
        system(("rm -f "+configpath+"TermColors").c_str());

        for(int s=0; s<screens; s++)
        {
        	system(("echo "+to_string(monitorXRes[s])+" >> "+configpath+"TermColors").c_str());
                system(("echo "+to_string(monitorYRes[s])+" >> "+configpath+"TermColors").c_str());
                system(("echo "+to_string(monitorXOff[s])+" >> "+configpath+"TermColors").c_str());
                system(("echo "+to_string(monitorYOff[s])+" >> "+configpath+"TermColors").c_str());
                system(("echo "+avgRHex[s]+avgGHex[s]+avgBHex[s]+" >> "+configpath+"TermColors").c_str());
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
			cout<<"Major color: Red ("<<Rctr<<")"<<endl;
			averageColors('r');
		}
		else if(Gctr>Rctr&&Gctr>Bctr)//Green
                {
			cout<<"Major color: Green ("<<Gctr<<")"<<endl;
                        averageColors('g');
                }
		else if(Gctr>Rctr&&Gctr>Bctr)//Blue
                {
			cout<<"Major color: Blue ("<<Bctr<<")"<<endl;
                        averageColors('b');
                }
		else
		{
			cout<<"Major color: Other"<<endl;
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
	cout<<"Average (hex): #"<<avgRHex[s]<<avgGHex[s]<<avgBHex[s]<<endl;

	//Save against all black text for troubleshooting
        if(avgGHex[s]=="0"&&avgGHex[s]=="0"&&avgBHex[s]=="0")
        {
        	cout<<"Black Save!"<<endl;
                avgRHex[s]="ff";
                avgGHex[s]="ff";
                avgBHex[s]="ff";
        }
	cout<<endl;
}
void foregroundColorApply(int m)
{
	//Set currently open terminals to the new color
	cout<<"Calling OSC escape sequences to change terminal colors on monitor "<<m<<endl;
	for(int i=0; i<maxTerms; i++)
	{
		if(term[i]!=0)
		{
			cout<<"Terminal "<<term[i]<<" check"<<endl;
			if(termX[i]>monitorXOff[m]&&termX[i]<(monitorXOff[m]+monitorXRes[m]))
			{
				cout<<"PASS: "<<termX[i]<<">"<<monitorXOff[m]<<"&&"<<termX[i]<<"<"<<monitorXOff[m]+monitorXRes[m]<<endl;
				if(termY[i]>monitorYOff[m]&&termY[i]<(monitorYOff[m]+monitorYRes[m]))
	                       	{
					cout<<"PASS: "<<termY[i]<<">"<<monitorYOff[m]<<"&&"<<termY[i]<<"<"<<monitorYOff[m]+monitorYRes[m]<<endl;

					cout<<"\t on "<<i<<endl;
      		 			system(("printf \"\033]10;#"+avgRHex[m]+avgGHex[m]+avgBHex[m]+"\007\" > /dev/pts/"+to_string(term[i])).c_str());
       	       			}
				else
				{
					cout<<"FAIL: "<<termY[i]<<"<="<<monitorYOff[m]<<"||"<<termY[i]<<">="<<monitorYOff[m]+monitorYRes[m]<<endl;
				}
			}
			else
               	 	{
               	        	 cout<<"FAIL: "<<termX[i]<<"<="<<monitorXOff[m]<<"||"<<termX[i]<<">="<<monitorXOff[m]+monitorXRes[m]<<endl;
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
                        	cout<<"Setting image path to "<<picpath<<endl;
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
        	                	cout<<"Setting steps to "<<steps<<endl;
				}
				else
				{
					cout<<"\""+line.substr(confIndex,(line.length()-confIndex))+"\" does not evenly divide into 100, using default step count"<<endl;
				}
                	}

			if(line.find("delay") != std::string::npos && !(line.find("sub") != std::string::npos))
                	{
                        	confIndex=line.find("=",0);

				//Convert string to int
                        	std::istringstream s ((line.substr(confIndex+1,(line.length()-confIndex))));
                       	 	s >> delay;

                       		cout<<"Setting delay to "<<delay<<" seconds"<<endl;
				delay*=second;
               		}

			if(line.find("subdelay") != std::string::npos)
        	        {
	                        confIndex=line.find("=",0);

                        	//Convert string to int
                        	std::istringstream s ((line.substr(confIndex+1,(line.length()-confIndex))));
                        	s >> subdelay;

                        	cout<<"Setting subdelay to "<<subdelay<<" miliseconds"<<endl;
                        	subdelay*=milisecond;
                	}

			if(line.find("threshold") != std::string::npos)
                        {
                                confIndex=line.find("=",0);

                                //Convert string to int
                                std::istringstream s ((line.substr(confIndex+1,(line.length()-confIndex))));
                                s >> threshold;

                                cout<<"Setting threshold to "<<threshold<<endl;
                        }

			if(line.find("colorForce") != std::string::npos)
                        {
                                colorForce=to_bool(line.substr(line.find("=",0)+1,(line.length()-(line.find("=",0)+1))));

                                cout<<"Setting colorForce to "<<std::boolalpha<<colorForce<<endl;
                        }

			if(line.find("fadeForeground") != std::string::npos)
                        {
                                fadeForeground=to_bool(line.substr(line.find("=",0)+1,(line.length()-(line.find("=",0)+1))));

                                cout<<"Setting fadeForeground to "<<std::boolalpha<<fadeForeground<<endl;
                        }
		}
        }
        readstream.close();
}

void makeConfig()
{
	cout<<configpath<<"config not found, creating..."<<endl;
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
	writestream.close();
}

int main(int argc, char **argv)
{
	InitializeMagick(*argv);

	//Get number of screens
	std::istringstream ss (getCmdOut("xrandr | grep -v disconnected | grep -o connected | wc -l"));
        ss >> screens;

	//Get monitor info
	cout<<"Monitor X resolutions"<<endl;
	for(int i=0; i<screens; i++)
	{
		std::istringstream ss (getCmdOut("xrandr | grep -E '[d|y] [0-9]{2,4}x[0-9]{2,4}' | grep -Eo '[0-9]{2,4}x[0-9]{2,4}' | grep -Eo '[0-9]{2,4}x' | grep -Eo '[0-9]{2,4}' | sed -n '"+to_string(i+1)+" p'"));
		ss >> monitorXRes[i];
		cout<<monitorXRes[i]<<endl;
	}
	cout<<"Monitor Y resolutions"<<endl;
        for(int i=0; i<screens; i++)
        {
                std::istringstream ss (getCmdOut("xrandr | grep -E '[d|y] [0-9]{2,4}x[0-9]{2,4}' | grep -Eo '[0-9]{2,4}x[0-9]{2,4}' | grep -Eo 'x[0-9]{2,4}' | grep -Eo '[0-9]{2,4}' | sed -n '"+to_string(i+1)+" p'"));
                ss >> monitorYRes[i];
                cout<<monitorYRes[i]<<endl;
        }
	cout<<"Monitor X offsets"<<endl;
	for(int i=0; i<screens; i++)
	{
		std::istringstream ss (getCmdOut("xrandr | grep -E '[d|y] [0-9]{2,4}x[0-9]{2,4}' | grep -Eo '[0-9]{2,4}x[0-9]{2,4}[+][0-9]{0,4}[+][0-9]{0,4}' | grep -Eo '[+][0-9]{0,4}[+]' | grep -Eo '[0-9]{0,4}' | sed -n '"+to_string(i+1)+" p'"));
                ss >> monitorXOff[i];
                cout<<monitorXOff[i]<<endl;
	}
	cout<<"Monitor Y offsets"<<endl;
        for(int i=0; i<screens; i++)
        {
                std::istringstream ss (getCmdOut("xrandr | grep -E '[d|y] [0-9]{2,4}x[0-9]{2,4}' | grep -Eo '[0-9]{2,4}x[0-9]{2,4}[+][0-9]{0,4}[+][0-9]{0,4}' | grep -Eo '[+][0-9]{0,4}( |$)' | grep -Eo '[0-9]{0,4}' | sed -n '"+to_string(i+1)+" p'"));
                ss >> monitorYOff[i];
                cout<<monitorYOff[i]<<endl;
        }

	//Check for config file and make one if it does not exist
	if(!fexists((configpath).c_str()))
	{
		cout<<configpath<<" not found, creating..."<<endl;
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
	cout<<"Starting updateTermInfo"<<endl;
	system("nohup updateTermInfo &");

	while(true)
	{
		for(int S=0; S<screens; S++)
		{
			//updateTermColors();

			//Determine which terminal is on what monitor
			cout<<"Getting terminal positions"<<endl;
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

				cout<<"\t/dev/pts/"<<term[i]<<" ("<<termID[i]<<"): "<<termX[i]<<","<<termY[i]<<endl;
			}
			readstream.close();

			cout<<"\nWorking on screen "<<S<<endl;
			cout<<"Loading list of pics from: "+picpath+".cache/.pics"<<endl;

			ctr=0;

			//Get pic names															
			readstream.open(picpath+".cache/.pics");
			if(readstream.fail())
			{
				cerr<<"ERROR: Unable to open \'"+picpath+".cache/.pics\'. Error code: "<<errno<<endl;
			}
			else
			{
				for(int i=0; getline(readstream,line); i++)
				{
					pics[i]=line;
					cout<<"\tPic name "<<i<<": "<<pics[i]<<endl;
					ctr++;
				}
				readstream.close();
			}

			cout<<endl;

			//Get monitor resolution
			if(oldpic=="")
			{
				resolution=getCmdOut("xrandr | grep connected | grep -v disconnected |  grep -Eo \"[0-9]{2,4}x[0-9]{0,4}+[0-9]{0,4}\" | sed -n '"+to_string(S+1)+" p'");
				cout<<"Desktop resolution: "<<resolution<<endl;

				//Get random image to start with
				srand(time(NULL));
				rndHold=rand()%ctr;
				rndHoldOld=rndHold;
				oldpic=picpath+pics[rndHold];
				background=Image(oldpic);
				cout<<"Using: "<<pics[rndHold]<<endl;

				//Scale to desktop resolution
				for(int i=0; i<screens; i++)
				{
					cout<<"Image size (original): "<<background.columns()<<"x"<<background.rows()<<endl;
					background.resize(resolution.substr(0,resolution.find("x"))+"x"+resolution.substr(resolution.find("x")+1,resolution.length()-(resolution.find("x")))+"!");
					background.write(picpath+".cache/resizeOld"+to_string(i)+".jpg");
					lastbackground=background;
					oldpic=picpath+".cache/resizeOld"+to_string(i)+".jpg";
					bgW=background.columns();
					bgH=background.rows();
					cout<<"Image size (resized): "<<bgW<<"x"<<bgH<<endl;

					//Display image
					cout<<"Setting screen "<<i<<endl;
					system(("nitrogen  --head="+to_string(i)+" --set-scaled "+picpath+".cache/resizeOld"+to_string(i)+".jpg").c_str());

					foregroundColorSet(i);
					foregroundColorApply(i);
					oldAVG[i]=AVG;
				}
			}
			else
			{
				resolution=getCmdOut("xrandr | grep connected | grep -v disconnected |  grep -Eo \"[0-9]{2,4}x[0-9]{0,4}+[0-9]{0,4}\" | sed -n '"+to_string(S+1)+" p'");
				cout<<"Desktop resolution: "<<resolution<<endl;

				//Get random new image
				srand(time(NULL));

				do
				{
					rndHold=rand()%ctr;
				}while(rndHold==rndHoldOld);

				rndHoldOld=rndHold;
				newpic=picpath+pics[rndHold];
				background=Image(newpic);
				cout<<"Using: "<<pics[rndHold]<<endl;

				//Scale to desktop resolution
				cout<<"Image size (original): "<<background.columns()<<"x"<<background.rows()<<endl;
				background.resize(resolution.substr(0,resolution.find("x"))+"x"+resolution.substr(resolution.find("x")+1,resolution.length()-(resolution.find("x")))+"!");
				background.write(picpath+".cache/resizeNew.jpg");
				newpic=picpath+".cache/resizeNew.jpg";
				bgW=background.columns();
				bgH=background.rows();
				cout<<"Image size (resized): "<<bgW<<"x"<<bgH<<endl;

				//Solve for transition steps
				for(int i=0; i<=steps; i++)
				{
					cout<<"Compositing transition step "<<i<<"... ";
					system(("composite -blend "+to_string((100/steps)*i)+" "+newpic+" "+picpath+".cache/resizeOld"+to_string(S)+".jpg"+" "+picpath+".cache/transition"+to_string(i)+".jpg").c_str());
					cout<<"Done!"<<endl;
					usleep(delay/steps);
				}

				if(fadeForeground)
				{
					foregroundColorSet(S);
				}

				//Fade new image in
				cout<<"Fading new image in on "<<S<<endl;
				for (int i=0; i<=steps; i++)
				{
					system(("nitrogen  --head="+to_string(S)+" --set-scaled "+picpath+".cache/transition"+to_string(i)+".jpg").c_str());

					if(fadeForeground)
					{
						//Solve for inbetween foreground colors
						fadePoint=(double)i/steps;
						cout<<"Fade point: "<<fadePoint<<endl;
						cout<<"Equation (red): "<<"((("<<AVG.red()*255<<")*"<<fadePoint<<")+(("<<oldAVG[S].red()*255<<")*(1-"<<fadePoint<<")))"<<endl;
						avgRTrans=(int)(((AVG.red()*255)*fadePoint)+((oldAVG[S].red()*255)*(1-fadePoint)));
						avgGTrans=(int)(((AVG.green()*255)*fadePoint)+((oldAVG[S].green()*255)*(1-fadePoint)));
						avgBTrans=(int)(((AVG.blue()*255)*fadePoint)+((oldAVG[S].blue()*255)*(1-fadePoint)));

						cout<<"Average (RGB): ("<<avgRTrans<<","<<avgGTrans<<","<<avgBTrans<<")"<<endl;

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
						cout<<"Average (hex): #"<<avgRHex[S]<<avgGHex[S]<<avgBHex[S]<<endl;

						//Save against all black text for troubleshooting
						if(avgGHex[S]=="0"&&avgGHex[S]=="0"&&avgBHex[S]=="0")
						{
							cout<<"Black Save!"<<endl;
							avgRHex[S]="ff";
							avgGHex[S]="ff";
							avgBHex[S]="ff";
						}
						cout<<endl;

						foregroundColorApply(S);
					}

					usleep(subdelay);
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

			cout<<"Done!\n"<<endl;

		}

	//clear cache
	system(("rm -f "+picpath+".cache/transition*").c_str());
	}

	XCloseDisplay(d);
	return 0;
}
