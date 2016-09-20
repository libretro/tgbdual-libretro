/*--------------------------------------------------
   TGB Dual - Gameboy Emulator -
   Copyright (C) 2001  Hii

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "gb.h"
#include <ctype.h>
#include <string.h>

cheat::cheat(gb *ref)
{
	ref_gb=ref;
	cheat_list.clear();
	create_cheat_map();
}

cheat::~cheat()
{
	cheat_list.clear();
}

void cheat::clear()
{
	cheat_list.clear();
	create_cheat_map();
}

void cheat::add_cheat(cheat_dat *dat)
{
	if (dat->code==0)
		ref_gb->get_cpu()->write(dat->adr,dat->dat);
	else{
		cheat_list.push_back(*dat);
		create_cheat_map();
	}
}

void cheat::delete_cheat(char *name)
{
	std::list<cheat_dat>::iterator ite;

	for (ite=cheat_list.begin();ite!=cheat_list.end();ite++){
		if (strcmp(ite->name,name)==0){
			cheat_list.erase(ite);
			break;
		}
	}

	create_cheat_map();
}

std::list<cheat_dat>::iterator cheat::find_cheat(char *name)
{
	std::list<cheat_dat>::iterator ite;

	for (ite=cheat_list.begin();ite!=cheat_list.end();ite++){
		if (strcmp(ite->name,name)==0){
			return ite;
		}
	}
	return ite;
}

void cheat::create_unique_name(char *buf)
{
	int num;
	bool end=false;
	char tmp[16];
	std::list<cheat_dat>::iterator ite;

	for (num=0;!end;num++){
		end=true;
		sprintf(tmp,"cheat_%03d",num);
		for (ite=cheat_list.begin();ite!=cheat_list.end();ite++)
			if (strcmp(ite->name,tmp)==0)
				end=false;
	}

	strcpy(buf,tmp);
}

void cheat::create_cheat_map()
{
	int i;
	std::list<cheat_dat>::iterator ite;
	cheat_dat *tmp;

	memset(cheat_map,0,sizeof(int)*0x10000);

	for (ite=cheat_list.begin();ite!=cheat_list.end();ite++){
		tmp=&(*ite);
		do{
			switch(tmp->code){
			case 0x01:
			case 0x90:
			case 0x91:
			case 0x92:
			case 0x93:
			case 0x94:
			case 0x95:
			case 0x96:
			case 0x97:
			case 0xA1:
				cheat_map[tmp->adr]=1;
				break;
			case 0x10:
				for (i=0;i<tmp->dat;i++)
					cheat_map[tmp->next->adr+(tmp->adr+1)*i]=1;
				tmp=tmp->next;
				break;
			}
			tmp=tmp->next;
		}while(tmp);
	}
}

byte cheat::cheat_read(word adr)
{
	std::list<cheat_dat>::iterator ite;
	cheat_dat *tmp;

	for (ite=cheat_list.begin();ite!=cheat_list.end();ite++){
		tmp=&(*ite);

		if (!tmp->enable)
			continue;

		do{
			switch(tmp->code){
			case 0x01:
				if (tmp->adr==adr)
					return tmp->dat;
				tmp=NULL;
				break;
			case 0x10:
				if ((tmp->next->adr<=adr)&&((adr-tmp->next->adr)<(tmp->adr+1)*tmp->dat)&&
					(((adr-tmp->next->adr)%(tmp->adr+1))==0))
					return tmp->next->dat;
				tmp=NULL;
				break;
			case 0x20:
				if (ref_gb->get_cpu()->read_direct(tmp->adr)==tmp->dat)
					tmp=tmp->next;
				else
					tmp=NULL;
				break;
			case 0x21:
				if (ref_gb->get_cpu()->read_direct(tmp->adr)<tmp->dat)
					tmp=tmp->next;
				else
					tmp=NULL;
				break;
			case 0x22:
				if (ref_gb->get_cpu()->read_direct(tmp->adr)>tmp->dat)
					tmp=tmp->next;
				else
					tmp=NULL;
				break;
			case 0x90:
			case 0x91:
			case 0x92:
			case 0x93:
			case 0x94:
			case 0x95:
			case 0x96:
			case 0x97:
				if (tmp->adr==adr)
					if ((adr>=0xD000)&&(adr<0xE000))
						if (((ref_gb->get_cpu()->get_ram_bank()-ref_gb->get_cpu()->get_ram())/0x1000)==(tmp->code-0x90))
							return tmp->dat;
						else
							tmp=NULL;
					else
						return tmp->dat;
				break;
			}
		}while(tmp);
	}

//	return 0;
	return ref_gb->get_cpu()->read_direct(adr);
}

void cheat::cheat_write(word adr,byte dat)
{
}

void cheat::save(FILE *file)
{
	std::list<cheat_dat>::iterator ite;
	cheat_dat *tmp;

	for (ite=cheat_list.begin();ite!=cheat_list.end();ite++){
		tmp=&(*ite);
		fprintf(file,"%s\n",tmp->name);
		do{
			fprintf(file,"%02X%02X%02X%02X\n",tmp->code,tmp->dat,tmp->adr&0xff,tmp->adr>>8);
			tmp=tmp->next;
		}while(tmp);
		fprintf(file,"\n");
	}
}

void cheat::load(FILE *file)
{
	cheat_list.clear();
	cheat_dat tmp_dat,*tmp=&tmp_dat;
	char buf[256];
	int i;
	bool first=true,req=false,ready=false;

	while(!feof(file)){
		if (!fgets(buf,256,file)){
			if (ready){
				tmp->next=NULL;
				tmp_dat.enable=true;
				add_cheat(&tmp_dat);
				first=true;
				tmp=&tmp_dat;
				req=false;
				ready=false;
			}
		}
		else if (buf[0]=='\n'){
			if (ready){
				tmp->next=NULL;
				tmp_dat.enable=true;
				add_cheat(&tmp_dat);
				first=true;
				tmp=&tmp_dat;
				req=false;
				ready=false;
			}
		}
		else{
			if (first){
				for (i=0;i<256;i++)
					if (buf[i]=='\n')
						buf[i]='\0';
				strcpy(tmp->name,buf);
				first=false;
				req=false;
			}
			else{
				for (i=0;i<256;i++)
					if (buf[i]=='\n')
						buf[i]='\0';
					else 
						buf[i]=toupper(buf[i]);

				if (req){
					tmp->next=new cheat_dat;
					tmp=tmp->next;
				}
				tmp->code=(isalpha(buf[0])?(buf[0]-'A'+10):(buf[0]-'0'))*16+(isalpha(buf[1])?(buf[1]-'A'+10):(buf[1]-'0'));
				tmp->adr=(isalpha(buf[4])?(buf[4]-'A'+10):(buf[4]-'0'))*16+(isalpha(buf[5])?(buf[5]-'A'+10):(buf[5]-'0'))+
					(isalpha(buf[6])?(buf[6]-'A'+10):(buf[6]-'0'))*4096+(isalpha(buf[7])?(buf[7]-'A'+10):(buf[7]-'0'))*256;
				tmp->dat=(isalpha(buf[2])?(buf[2]-'A'+10):(buf[2]-'0'))*16+(isalpha(buf[3])?(buf[3]-'A'+10):(buf[3]-'0'));
				req=true;
				ready=true;
			}
		}
	}
	if (ready){
		tmp->next=NULL;
		tmp_dat.enable=true;
		add_cheat(&tmp_dat);
		first=true;
		tmp=&tmp_dat;
		req=false;
		ready=false;
	}
}
