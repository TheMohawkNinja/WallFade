#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <climits>

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
	const unsigned short int maxTerms=UINT8_MAX;
	const unsigned short int maxMonitors=UINT8_MAX;
	int term[maxTerms],termX[maxTerms],termY[maxTerms];
	int monitorXOff[maxMonitors],monitorYOff[maxMonitors],monitorXRes[maxMonitors],monitorYRes[maxMonitors];
	string line,tty;
	string user=getlogin();
	string configpath="/home/"+user+"/.config/WallFade/";
	string termColor[maxTerms];
	string termID[maxTerms];
	ifstream readstream;

	//while(true)
	while(getCmdOut("ps -aux | grep WallFade | grep -v grep")!="")
	{

		//Reset terminal arrays
		for(int i=0; i<maxTerms; i++)
		{
			term[i]=0;
			termID[i]="";
			termX[i]=0;
			termY[i]=0;
		}

		cout<<"Getting terminal positions"<<endl;
	        readstream.open(configpath+"TermInfo");
	        for(int i=0; !readstream.eof()&&readstream; i++)
	        {
			getline(readstream,line);
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

			if(term[i]!=0)
			{
				cout<<"\t"+to_string(term[i])+", "+termID[i]+", "+to_string(termX[i])+", "+to_string(termY[i])<<endl;
			}
			else
			{
				break;
			}
		}
	        readstream.close();

		cout<<endl;
		cout<<"Updating term info"<<endl;
		system(("rm -f "+configpath+"TermInfo").c_str());
		system(("ls /dev/pts | grep -E '[1-9]{1,9}' > "+configpath+"tty").c_str());
		for(int i=0; term[i]!=0; i++)
		{
			readstream.open(configpath+"tty");
			while(readstream)
			{
				//Check if terminal still exists, remove duplicates, and write back to TermInfo
				getline(readstream,line);
				std::istringstream stty (line);
				stty >> tty;

				if(stoi(tty)==term[i])
				{
					cout<<"Terminal "+to_string(term[i])+" located ("+termID[i]+"), checking for duplicates"<<endl;

					//Check for duplicate entries and remove if they exist
					for(int k=i+1; term[k]!=0; k++)
					{
						if(term[k]==term[i])
						{
							cout<<"\tRemoving duplicate entry (tty "+to_string(term[k])+", "+termID[k]+")"<<endl;
							for(int j=i; term[j]!=0; j++)
							{
								cout<<"\tSetting term["+to_string(j)+"] to "+to_string(term[j+1])<<endl;
								if(term[j+1]==0)
								{
									term[j]=0;
									termID[j]="";
									termX[j]=0;
									termY[j]=0;
									break;
								}
								else
								{
									term[j]=term[j+1];
									termID[j]=termID[j+1];
									termX[j]=termX[j+1];
									termY[j]=termY[j+1];
								}
							}
						}
					}

					system(("echo "+to_string(term[i])+" >> "+configpath+"TermInfo").c_str());
					system(("echo "+termID[i]+" >> "+configpath+"TermInfo").c_str());

					std::istringstream sx (getCmdOut(("xwininfo -id "+termID[i]+" | grep -E 'Absolute upper-left X:  [0-9]{0,6}' | grep -Eo '[0-9]{0,6}'").c_str()));
              		        	sx >> termX[i];
					system(("echo "+to_string(termX[i])+" >> "+configpath+"TermInfo").c_str());

					std::istringstream sy (getCmdOut(("xwininfo -id "+termID[i]+" | grep -E 'Absolute upper-left Y:  [0-9]{0,6}' | grep -Eo '[0-9]{0,6}'").c_str()));
                    			sx >> termY[i];
        		                system(("echo "+to_string(termY[i])+" >> "+configpath+"TermInfo").c_str());

					cout<<"\tRe-cached term "<<term[i]<<" ("<<termID[i]<<") at: "<<termX[i]<<", "<<termY[i]<<endl;

					readstream.close();
					break;
				}

				//If terminal doesn't exist, delete it and sort
				if(readstream.eof())
				{
					cout<<"Terminal "+to_string(term[i])+" NOT located, deleting"<<endl;

					for(int j=i; term[j]!=0; j++)
					{
						cout<<"\tSetting term["+to_string(j)+"] to "+to_string(term[j+1])<<endl;
						if(term[j+1]==0)
						{
							term[j]=0;
							termID[j]="";
							termX[j]=0;
							termY[j]=0;
							break;
						}
						else
						{
							term[j]=term[j+1];
							termID[j]=termID[j+1];
							termX[j]=termX[j+1];
							termY[j]=termY[j+1];
						}
					}

					i--;

					cout<<endl;
					cout<<"TERM ORDER"<<endl;
					for(int x=0; term[x]!=0; x++)
					{
						cout<<to_string(term[x])+" ";
					}
					cout<<endl;

					readstream.close();
					break;
				}
			}
		}
		cout<<endl;
		sleep(1);
	}

	return 0;
}
