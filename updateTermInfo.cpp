#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>

using namespace std;

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

int main()
{
	const int maxTerms=100;
	const int maxMonitors=8;
	int term[maxTerms],termX[maxTerms],termY[maxTerms];
	int monitorXOff[maxMonitors],monitorYOff[maxMonitors],monitorXRes[maxMonitors],monitorYRes[maxMonitors];
	string line;
	string user=getlogin();
	string configpath="/home/"+user+"/.config/WallFade/";
	string termColor[maxTerms];
	string termID[maxTerms];
	ifstream readstream;

	//while(true)
	while(getCmdOut("ps -aux | grep WallFade | grep -v grep")!="")
	{
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
	        }
	        readstream.close();

		cout<<"Updating term info"<<endl;
		system(("rm -f "+configpath+"TermInfo").c_str());
		for(int i=0; term[i]!=0; i++)
		{
			system(("echo "+to_string(term[i])+" >> "+configpath+"TermInfo").c_str());
			system(("echo "+termID[i]+" >> "+configpath+"TermInfo").c_str());

			std::istringstream sx (getCmdOut(("xwininfo -id "+termID[i]+" | grep -E 'Absolute upper-left X:  [0-9]{0,6}' | grep -Eo '[0-9]{0,6}'").c_str()));
                        sx >> termX[i];
			system(("echo "+to_string(termX[i])+" >> "+configpath+"TermInfo").c_str());

			std::istringstream sy (getCmdOut(("xwininfo -id "+termID[i]+" | grep -E 'Absolute upper-left Y:  [0-9]{0,6}' | grep -Eo '[0-9]{0,6}'").c_str()));
                        sx >> termY[i];
                        system(("echo "+to_string(termY[i])+" >> "+configpath+"TermInfo").c_str());

			cout<<"Term "<<term[i]<<" ("<<termID[i]<<") at: "<<termX[i]<<", "<<termY[i]<<endl;
		}

		cout<<"Getting terminal colors"<<endl;
		readstream.open(configpath+"TermColors");
		for(int i=0; getline(readstream,line); i++)
	        {
			std::istringstream sxr (line);
                        sxr >> monitorXRes[i];

			getline(readstream,line);
			std::istringstream syr (line);
                        syr >> monitorYRes[i];

			getline(readstream,line);
                        std::istringstream sxo (line);
                        sxo >> monitorXOff[i];

			getline(readstream,line);
                        std::istringstream syo (line);
                        syo >> monitorYOff[i];

			getline(readstream,line);
			termColor[i]=line;

			cout<<"Monitor "<<i<<"="<<"("<<monitorXRes[i]<<","<<monitorYRes[i]<<","<<monitorXOff[i]<<","<<monitorYOff[i]<<",#"<<termColor[i]<<")"<<endl;
		}
		readstream.close();
		cout<<"Applying colors"<<endl;
		for(int m=0; termColor[m]!=""; m++)
		{
			cout<<"termColor["<<m<<"]="<<termColor[m]<<endl;
			for(int i=0; termID[i]!=""; i++)
			{
				cout<<"termID["<<m<<"]="<<termID[m]<<endl;
				if(termX[i]>monitorXOff[m]&&termX[i]<(monitorXOff[m]+monitorXRes[m]))
                        	{
                               	 	if(termY[i]>monitorYOff[m]&&termY[i]<(monitorYOff[m]+monitorYRes[m]))
                                	{
						cout<<"\tColor of term "<<term[i]<<" on monitor "<<m<<"="<<termColor[m]<<endl;
						system(("printf \"\033]10;#"+termColor[m]+"\007\" > /dev/pts/"+to_string(term[i])).c_str());
					}
				}
			}
		}
		cout<<endl;
		usleep(100000);
	}

	return 0;
}
