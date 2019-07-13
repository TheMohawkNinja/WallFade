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

using namespace std;
using namespace Magick;

ifstream readstream;
ofstream writestream;
string configpath="/etc/WallFade/";
string line;
string user=getlogin();
string path="/home/"+user+"/Pictures/";
string::size_type index;
int index_int, steps, delay, subdelay;
int milisecond=1000;
int second=1000*milisecond;
int threshold=100;

bool fexists(const char *filename)
{
  ifstream ifile(filename);
  return (bool)ifile;
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
				index=line.find("=",0);
				path=line.substr(index+1,(line.length()-index));
                        	cout<<"Setting image path to "<<path<<endl;
                	}

			if(line.find("steps") != std::string::npos)
                	{
                	        index=line.find("=",0);

				//Convert string to int
				std::istringstream s ((line.substr(index+1,(line.length()-index))));
				s >> index_int;

				//If the user's step count is evenly divisible from 100, use it
				if(100%index_int==0)
				{
        	                	steps=index_int;
        	                	cout<<"Setting steps to "<<steps<<endl;
				}
				else
				{
					cout<<"\""+line.substr(index,(line.length()-index))+"\" does not evenly divide into 100, using default step count"<<endl;
				}
                	}

			if(line.find("delay") != std::string::npos && !(line.find("sub") != std::string::npos))
                	{
                        	index=line.find("=",0);

				//Convert string to int
                        	std::istringstream s ((line.substr(index+1,(line.length()-index))));
                       	 	s >> delay;

                       		cout<<"Setting delay to "<<delay<<" seconds"<<endl;
				delay*=second;
               		}

			if(line.find("subdelay") != std::string::npos)
        	        {
	                        index=line.find("=",0);

                        	//Convert string to int
                        	std::istringstream s ((line.substr(index+1,(line.length()-index))));
                        	s >> subdelay;

                        	cout<<"Setting subdelay to "<<subdelay<<" miliseconds"<<endl;
                        	subdelay*=milisecond;
                	}

			if(line.find("threshold") != std::string::npos)
                        {
                                index=line.find("=",0);

                                //Convert string to int
                                std::istringstream s ((line.substr(index+1,(line.length()-index))));
                                s >> threshold;

                                cout<<"Setting threshold to "<<threshold<<endl;
                        }
		}
        }
        readstream.close();

}

void makeConfig()
{
	cout<<configpath<<"config not found, creating..."<<endl;
	writestream.open((configpath+"config").c_str());
	writestream<<"#Path where your wallpapers are located. Default: "+path+"\n";
	writestream<<"path="+path+"\n\n";
	writestream<<"#Number of transitions steps for fade sequence. More steps  means more cached images, but a smoother transition. Default: "+to_string(steps)+"\n";
	writestream<<"steps="+to_string(steps)+"\n\n";
	writestream<<"#Number of seconds between wallpaper change. Default: "+to_string(delay/second)+"\n";
	writestream<<"delay="+to_string(delay/second)+"\n\n";
	writestream<<"#Number of miliseconds between each transition step. Will make fade last longer, but fade will look choppy if this value is too high. Default: "+to_string(subdelay/milisecond)+"\n";
	writestream<<"subdelay="+to_string(subdelay/milisecond)+"\n\n";
	writestream<<"#The threshold for each 8-bit color channel that the system will use to determine which pixels will be selected for determining terminal color. Default: "+to_string(threshold)+"\n";
        writestream<<"threshold="+to_string(threshold)+"\n";
	writestream.close();
}

int main(int argc, char **argv)
{
	InitializeMagick(*argv);
	int bgW, bgH, r, g, b, ctr, rndHold, rndHoldOld, threshold;
	double avgR, avgG, avgB;
	string avgRHex;
	string avgGHex;
	string avgBHex;
	string newpic;
	string oldpic;
	string bgpath;
	string pics[100];
	Image ColorSpace(Geometry(60,30),Color(255,255,255));
	Image mask(Geometry(60,30),Color(0,0,0));
	Image background;
	Image lastbackground;
	ColorRGB ColorSample[30][30];
	ColorRGB AVG;
	stringstream stream;
	Display* d=XOpenDisplay(NULL);
	Screen* s=DefaultScreenOfDisplay(d);

	threshold=100;
	steps=25;
	delay=60*second;

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
	if(!fexists("/etc/WallFade/config"))
        {
		makeConfig();
		readConfig();
        }
        else
        {
		readConfig();
        }

	//Make cache directory
	if(!fexists((path+".cache").c_str()))
	{
		system((path+".cache").c_str());
	}

	//Make list of pics
	system(("cd "+path+" && ls | grep -E 'jpg|png' | grep -Ev 'transition|resize' > .cache/.pics").c_str());

	while(true)
	{
	ctr=0;

	//Get pic names
	readstream.open(path+".cache/.pics");

	for(int i=0; getline(readstream,line); i++)
	{
		pics[i]=line;
		cout<<"Pic name "<<i<<": "<<pics[i]<<endl;
		ctr++;
	}
	readstream.close();

	cout<<"Desktop resolution: "<< s->width <<"x"<< s->height <<endl;

	if(oldpic=="")
	{
		//Get random image to start with
       	 	srand(time(NULL));
        	rndHold=rand()%ctr;
		rndHoldOld=rndHold;
		oldpic=path+pics[rndHold];
		background=Image(oldpic);
		cout<<"Using :"<<pics[rndHold]<<endl;

		//Scale to desktop resolution
		cout<<"Image size (original): "<<background.columns()<<", "<<background.rows()<<endl;
		background.resize(to_string(s->width)+"x"+to_string(s->height)+"!");
		background.write(path+".cache/resizeOld.jpg");
		lastbackground=background;
		oldpic=path+".cache/resizeOld.jpg";
		bgW=background.columns();
        	bgH=background.rows();
		cout<<"Image size (resized): "<<bgW<<", "<<bgH<<endl;

		//Display image
		system(("feh --bg-scale "+oldpic).c_str());
	}
	else
	{
		//Get random new image
		srand(time(NULL));

		do
		{
			rndHold=rand()%ctr;
		}while(rndHold==rndHoldOld);

		rndHoldOld=rndHold;
		newpic=path+pics[rndHold];
		background=Image(newpic);
		cout<<"Using :"<<pics[rndHold]<<endl;

		//Scale to desktop resolution
		cout<<"Image size (original): "<<background.columns()<<", "<<background.rows()<<endl;
		background.resize(to_string(s->width)+"x"+to_string(s->height)+"!");
		background.write(path+".cache/resizeNew.jpg");
		newpic=path+".cache/resizeNew.jpg";
		bgW=background.columns();
        	bgH=background.rows();
		cout<<"Image size (resized): "<<bgW<<", "<<bgH<<endl;

		//Solve for transition steps
		for (int i=0; i<steps; i++)
		{
			cout<<"Compositing transition step "<<i<<endl;
			system(("composite -blend "+to_string((100/steps)*i)+" "+newpic+" "+path+".cache/resizeOld.jpg"+" "+path+".cache/transition"+to_string(i)+".jpg").c_str());
			//cout<<"Sleeping for "<<to_string(delay/steps)<<" microseconds"<<endl;
			usleep(delay/steps);
		}

		//Fade new image in
		cout<<"Fading new image in"<<endl;
		for (int i=0; i<steps; i++)
		{
			system(("feh --bg-scale "+path+".cache/transition"+to_string(i)+".jpg").c_str());
			//cout<<"Sleeping for "<<to_string((second/steps)+subdelay)<<" microseconds"<<endl;
			usleep((second/steps)+subdelay);
		}

		//Set "new" old picture
		oldpic=newpic;
		lastbackground=background;;
		system(("cp "+path+".cache/resizeNew.jpg"+" "+path+".cache/resizeOld.jpg").c_str());
	}

	//Make 30*30 sample space from image
	for(int y=0; y<30; y++)
	{
		for(int x=0; x<30; x++)
        	{
			ColorSample[x][y]=background.pixelColor(((bgW/30)*x),((bgH/30)*y));
			r=ColorSample[x][y].red()*255;
			g=ColorSample[x][y].green()*255;
			b=ColorSample[x][y].blue()*255;

			if(r>threshold || g>threshold || b>threshold)
			{
				ColorSpace.pixelColor(x,y,ColorSample[x][y]);
				avgR+=r;
				avgG+=g;
				avgB+=b;
				ctr++;
       			}
		}
	}

	//Get average of saturated colors
	avgR/=(ctr*255);
	avgG/=(ctr*255);
	avgB/=(ctr*255);
	cout<<"Average (RGB): ("<<(int)(avgR*255)<<","<<(int)(avgG*255)<<","<<(int)(avgB*255)<<")"<<endl;
	AVG=ColorRGB(avgR,avgG,avgB);

	for(int y=0; y<30; y++)
        {
                for(int x=0; x<30; x++)
                {
			ColorSpace.pixelColor(x+30,y,AVG);
		}
	}

	//Convert averages to hex
	stream<<std::hex<<(int)(avgR*255);
	avgRHex=stream.str();
	stream.str("");
	stream<<std::hex<<(int)(avgG*255);
        avgGHex=stream.str();
	stream.str("");
	stream<<std::hex<<(int)(avgB*255);
        avgBHex=stream.str();
	stream.str("");
	cout<<"Average (hex): "<<avgRHex<<avgGHex<<avgBHex<<endl;

	//Write new .Xresources file
	readstream.open("/home/"+user+"/.Xresources");
	writestream.open("/home/"+user+"/.Xresources.tmp");
	for(int i=0; getline(readstream,line); i++)
        {
		if(line.find("foreground") != std::string::npos)
		{
			cout<<"Changing .Xresources foreground color"<<endl;
			writestream<<"*.foreground: #"+avgRHex+avgGHex+avgBHex+"\n";
		}
		else
		{
			writestream<<line+"\n";
		}
        }
	readstream.close();
	writestream.close();

	//Replace old .Xresources file with new one and load it
	system(("cp /home/"+user+"/.Xresources.tmp /home/$USER/.Xresources").c_str());
	system(("xrdb /home/"+user+"/.Xresources").c_str());

	//Set currently open terminals to the new color
	cout<<"Calling OSC escape sequence to change terminal color"<<endl;;
	for(int i=0; fexists(("/dev/pts/"+to_string(i)).c_str()); i++)
	{
		cout<<"\t on "<<i<<endl;
		system(("printf \"\033]10;#"+avgRHex+avgGHex+avgBHex+"\007\" > /dev/pts/"+to_string(i)).c_str());
	}

	cout<<"Done!\n"<<endl;
}//while end
	return 0;
}
