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

//-------------------------------------------------
// GB その他エミュレーション部/外部とのインターフェース
// Interface with external / other unit emulation GB

#include "gb.h"
#include <stdlib.h>
#include <memory.h>

gb::gb(renderer *ref,bool b_lcd,bool b_apu)
{
	m_renderer=ref;

	m_lcd=new lcd(this);
	m_rom=new rom();
	m_apu=new apu(this);// ROMより後に作られたし // I was made ​​later than the ROM
	m_mbc=new mbc(this);
	m_cpu=new cpu(this);
	m_cheat=new cheat(this);
	target=NULL;

	m_renderer->reset();
	m_renderer->set_sound_renderer(b_apu?m_apu->get_renderer():NULL);

	reset();

	hook_ext=false;
	use_gba=false;
}

gb::~gb()
{
	m_renderer->set_sound_renderer(NULL);

	delete m_mbc;
	delete m_rom;
	delete m_apu;
	delete m_lcd;
	delete m_cpu;
}

void gb::reset()
{
	regs.SC=0;
	regs.DIV=0;
	regs.TIMA=0;
	regs.TMA=0;
	regs.TAC=0;
	regs.LCDC=0x91;
	regs.STAT=0;
	regs.SCY=0;
	regs.SCX=0;
	regs.LY=153;
	regs.LYC=0;
	regs.BGP=0xFC;
	regs.OBP1=0xFF;
	regs.OBP2=0xFF;
	regs.WY=0;
	regs.WX=0;
	regs.IF=0;
	regs.IE=0;

	memset(&c_regs,0,sizeof(c_regs));

	if (m_rom->get_loaded())
		m_rom->get_info()->gb_type=(m_rom->get_rom()[0x143]&0x80)?(use_gba?4:3):1;

	m_cpu->reset();
	m_lcd->reset();
	m_apu->reset();
	m_mbc->reset();

	now_frame=0;
	skip=skip_buf=0;
	re_render=0;

	char *gb_names[]={"Invalid","Gameboy","SuperGameboy","Gameboy Color","Gameboy Advance"};
	if (m_rom->get_loaded())
		m_renderer->output_log("Current GB Type : %s \n",gb_names[m_rom->get_info()->gb_type]);
}

void gb::hook_extport(ext_hook *ext)
{
	hook_proc=*ext;
	hook_ext=true;
}

void gb::unhook_extport()
{
	hook_ext=false;
}

void gb::set_skip(int frame)
{
	skip_buf=frame;
}

bool gb::load_rom(byte *buf,int size,byte *ram,int ram_size)
{
	if (m_rom->load_rom(buf,size,ram,ram_size)){
		reset();
		return true;
	}
	else
		return false;
}

void gb::save_state(FILE *file)
{
	int tbl_ram[]={1,1,1,4,16,8}; // 0と1は保険 // 0 and 1 insurance
	int has_bat[]={0,0,0,1,0,0,1,0,0,1,0,0,1,1,0,1,1,0,0,1,0,0,0,0,0,0,0,1,0,1,1,0}; // 0x20以下 // Less than 0x20

	// ゲームボーイの種類 (GB:1,SGB:2,GBC:3 …)
	// Type of Game Boy (GB: 1, SGB: 2, GBC: 3...)
	fwrite(&m_rom->get_info()->gb_type,sizeof(int),1,file);

	if (m_rom->get_info()->gb_type==1){ // normal gb
		fwrite(m_cpu->get_ram(),1,0x2000,file); // ram
		fwrite(m_cpu->get_vram(),1,0x2000,file); // vram
		fwrite(m_rom->get_sram(),1,tbl_ram[m_rom->get_info()->ram_size]*0x2000,file); // sram
		fwrite(m_cpu->get_oam(),1,0xA0,file);
		fwrite(m_cpu->get_stack(),1,0x80,file);

		int page,ram_page;
		page=(m_mbc->get_rom()-m_rom->get_rom())/0x4000;
		ram_page=(m_mbc->get_sram()-m_rom->get_sram())/0x2000;

		fwrite(&page,sizeof(int),1,file); // rom_page
		fwrite(&ram_page,sizeof(int),1,file); // ram_page

		int dmy=0;

		fwrite((const void *)m_cpu->get_regs(),sizeof(cpu_regs),1,file); // cpu_reg
		fwrite((const void *)&regs,sizeof(gb_regs),1,file);//sys_reg
		int halt=((*m_cpu->get_halt())?1:0);
		fwrite((const void *)&halt,sizeof(int),1,file);
		// 元の版ではシリアル通信通信満了までのクロック数
		// (通信の仕様が大幅に変わったためダミーで埋めている)
		// In the original version (which is filled with dummy for the specification of communication has changed significantly) until the expiration of the number of clock communication serial communication
		fwrite((const void *)&dmy,sizeof(int),1,file);
		int mbc_dat=m_mbc->get_state();
		fwrite(&mbc_dat,sizeof(int),1,file);//MBC

		int ext_is=m_mbc->is_ext_ram()?1:0;
		fwrite(&ext_is,sizeof(int),1,file);

		// ver 1.1 追加 // Added ver 1.1
		fwrite(m_apu->get_stat(),sizeof(apu_stat),1,file);
		fwrite(m_apu->get_mem(),1,0x30,file);
		fwrite(m_apu->get_stat_cpy(),sizeof(apu_stat),1,file);

		byte resurved[256];
		memset(resurved,0,256);
		fwrite(resurved,1,256,file);//将来のために確保 // Reserved for future use
	}
	else if (m_rom->get_info()->gb_type>=3){ // GB Colour / GBA
		fwrite(m_cpu->get_ram(),1,0x2000*4,file); // ram
		fwrite(m_cpu->get_vram(),1,0x2000*2,file); // vram
		fwrite(m_rom->get_sram(),1,tbl_ram[m_rom->get_info()->ram_size]*0x2000,file); // sram
		fwrite(m_cpu->get_oam(),1,0xA0,file);
		fwrite(m_cpu->get_stack(),1,0x80,file);

		int cpu_dat[16];
		m_cpu->save_state(cpu_dat);

		int page,ram_page;
		page=(m_mbc->get_rom()-m_rom->get_rom())/0x4000;
		ram_page=(m_mbc->get_sram()-m_rom->get_sram())/0x2000;

		fwrite(&page,sizeof(int),1,file); // rom_page
		fwrite(&ram_page,sizeof(int),1,file); // ram_page
		fwrite(cpu_dat+0,sizeof(int),1,file);//int_page
		fwrite(cpu_dat+1,sizeof(int),1,file);//vram_page

		int dmy=0;

		fwrite((const void *)m_cpu->get_regs(),sizeof(cpu_regs),1,file); // cpu_reg
		fwrite((const void *)&regs,sizeof(gb_regs),1,file);//sys_reg
		fwrite((const void *)&c_regs,sizeof(gbc_regs),1,file);//col_reg
		fwrite(m_lcd->get_pal(0),sizeof(word),8*4*2,file);//palette
		int halt=((*m_cpu->get_halt())?1:0);
		fwrite((const void *)&halt,sizeof(int),1,file);
		// 元の版ではシリアル通信通信満了までのクロック数
		// In the original version of the number of clocks until the expiration communication serial communication
		fwrite((const void *)&dmy,sizeof(int),1,file);

		int mbc_dat=m_mbc->get_state();
		fwrite(&mbc_dat,sizeof(int),1,file);//MBC

		int ext_is=m_mbc->is_ext_ram()?1:0;
		fwrite(&ext_is,sizeof(int),1,file);

		//その他諸々 // Many other countries
		fwrite(cpu_dat+2,sizeof(int),1,file);
		fwrite(cpu_dat+3,sizeof(int),1,file);
		fwrite(cpu_dat+4,sizeof(int),1,file);
		fwrite(cpu_dat+5,sizeof(int),1,file);
		fwrite(cpu_dat+6,sizeof(int),1,file);
		fwrite(cpu_dat+7,sizeof(int),1,file);

		// ver 1.1 追加 // Added ver 1.1
		fwrite(m_apu->get_stat(),sizeof(apu_stat),1,file);
		fwrite(m_apu->get_mem(),1,0x30,file);
		fwrite(m_apu->get_stat_cpy(),sizeof(apu_stat),1,file);

		byte resurved[256],reload=1;
		memset(resurved,0,256);
//		resurved[0]=1;
		fwrite(&reload,1,1,file);
		fwrite(resurved,1,256,file);//将来のために確保 // Reserved for future use
	}
}

void gb::restore_state(FILE *file)
{
	int tbl_ram[]={1,1,1,4,16,8}; // 0と1は保険 // 0 and 1 insurance
	int has_bat[]={0,0,0,1,0,0,1,0,0,1,0,0,1,1,0,1,1,0,0,1,0,0,0,0,0,0,0,1,0,1,1,0}; // 0x20以下 // Less than 0x20
	int gb_type,dmy;

	fread(&gb_type,sizeof(int),1,file);

	m_rom->get_info()->gb_type=gb_type;

	if (gb_type==1){
		fread(m_cpu->get_ram(),1,0x2000,file); // ram
		fread(m_cpu->get_vram(),1,0x2000,file); // vram
		fread(m_rom->get_sram(),1,tbl_ram[m_rom->get_info()->ram_size]*0x2000,file); // sram
		fread(m_cpu->get_oam(),1,0xA0,file);
		fread(m_cpu->get_stack(),1,0x80,file);

		int page,ram_page;
		fread(&page,sizeof(int),1,file); // rom_page
		fread(&ram_page,sizeof(int),1,file); // ram_page
		m_mbc->set_page(page,ram_page);

		fread(m_cpu->get_regs(),sizeof(cpu_regs),1,file); // cpu_reg
		fread((void *)&regs,sizeof(gb_regs),1,file); // sys_reg
		int halt;
		fread(&halt,sizeof(int),1,file);
		*m_cpu->get_halt()=((halt)?true:false);
		fread(&dmy,sizeof(int),1,file);

		int mbc_dat;
		fread(&mbc_dat,sizeof(int),1,file); // MBC
		m_mbc->set_state(mbc_dat);
		int ext_is;
		fread(&ext_is,sizeof(int),1,file);
		m_mbc->set_ext_is(ext_is?true:false);

		// ver 1.1 追加 // Added ver 1.1
		byte tmp[256],tester[100];
		fread(tmp,1,100,file); // とりあえず調べてみる // I'll try to find out anyway
		memset(tester,0,100);
		if (memcmp(tmp,tester,100)!=0){
			// apu 部分 // apu part
			fseek(file,-100,SEEK_CUR);
			fread(m_apu->get_stat(),sizeof(apu_stat),1,file);
			fread(m_apu->get_mem(),1,0x30,file);
			fread(m_apu->get_stat_cpy(),sizeof(apu_stat),1,file);
		}

		byte resurved[256];
		fread(resurved,1,256,file);//将来のために確保 // Reserved for future use
	}
	else if (gb_type>=3){ // GB Colour / GBA
		fread(m_cpu->get_ram(),1,0x2000*4,file); // ram
		fread(m_cpu->get_vram(),1,0x2000*2,file); // vram
		fread(m_rom->get_sram(),1,tbl_ram[m_rom->get_info()->ram_size]*0x2000,file); // sram
		fread(m_cpu->get_oam(),1,0xA0,file);
		fread(m_cpu->get_stack(),1,0x80,file);

		int cpu_dat[16];

		int page,ram_page;
		fread(&page,sizeof(int),1,file); // rom_page
		fread(&ram_page,sizeof(int),1,file); // ram_page
		m_mbc->set_page(page,ram_page);
		page=(m_mbc->get_rom()-m_rom->get_rom())/0x4000;
		ram_page=(m_mbc->get_sram()-m_rom->get_sram())/0x2000;

		fread(cpu_dat+0,sizeof(int),1,file);//int_page
		fread(cpu_dat+1,sizeof(int),1,file);//vram_page

		int dmy;
		fread(m_cpu->get_regs(),sizeof(cpu_regs),1,file); // cpu_reg
		fread(&regs,sizeof(gb_regs),1,file);//sys_reg
		fread(&c_regs,sizeof(gbc_regs),1,file);//col_reg
		fread(m_lcd->get_pal(0),sizeof(word),8*4*2,file);//palette
		int halt;
		fread(&halt,sizeof(int),1,file);
		*m_cpu->get_halt()=(halt?true:false);
		// 元の版ではシリアル通信通信満了までのクロック数
		// In the original version of the number of clocks until the expiration communication serial communication
		fread(&dmy,sizeof(int),1,file);

		int mbc_dat;
		fread(&mbc_dat,sizeof(int),1,file); // MBC
		m_mbc->set_state(mbc_dat);
		int ext_is;
		fread(&ext_is,sizeof(int),1,file);
		m_mbc->set_ext_is(ext_is?true:false);

		//その他諸々 // Many other countries
		fread(cpu_dat+2,sizeof(int),1,file);
		fread(cpu_dat+3,sizeof(int),1,file);
		fread(cpu_dat+4,sizeof(int),1,file);
		fread(cpu_dat+5,sizeof(int),1,file);
		fread(cpu_dat+6,sizeof(int),1,file);
		fread(cpu_dat+7,sizeof(int),1,file);
		m_cpu->restore_state(cpu_dat);

		// ver 1.1 追加 // Added ver 1.1
		byte tmp[256],tester[100];
		fread(tmp,1,100,file); // とりあえず調べてみる // I'll try to find out anyway
		memset(tester,0,100);
		if (memcmp(tmp,tester,100)!=0){
			// apu 部分
			fseek(file,-100,SEEK_CUR);
			fread(m_apu->get_stat(),sizeof(apu_stat),1,file);
			fread(m_apu->get_mem(),1,0x30,file);
			fread(m_apu->get_stat_cpy(),sizeof(apu_stat),1,file);

			fread(tmp,1,1,file);
			int i;
			if (tmp[0])
				for (i=0;i<64;i++)
					m_lcd->get_mapped_pal(i>>2)[i&3]=m_renderer->map_color(m_lcd->get_pal(i>>2)[i&3]);
			else{
				for (i=0;i<64;i++)
					m_lcd->get_pal(i>>2)[i&3]=m_renderer->unmap_color(m_lcd->get_pal(i>>2)[i&3]);
				for (i=0;i<64;i++)
					m_lcd->get_mapped_pal(i>>2)[i&3]=m_renderer->map_color(m_lcd->get_pal(i>>2)[i&3]);
			}
		}
		byte resurved[256];
		fread(resurved,1,256,file);//将来のために確保 // Reserved for future use
	}
}

void gb::refresh_pal()
{
	for (int i=0;i<64;i++)
		m_lcd->get_mapped_pal(i>>2)[i&3]=m_renderer->map_color(m_lcd->get_pal(i>>2)[i&3]);
}

void gb::run()
{
	if (m_rom->get_loaded()){
		if (regs.LCDC&0x80){ // LCDC 起動時 // Startup LCDC
			regs.LY=(regs.LY+1)%154;

			regs.STAT&=0xF8;
			if (regs.LYC==regs.LY){
				regs.STAT|=4;
				if (regs.STAT&0x40)
					m_cpu->irq(INT_LCDC);
			}
			if (regs.LY==0){
				m_renderer->refresh();
				if (now_frame>=skip){
					m_renderer->render_screen((byte*)vframe,160,144,16);
					now_frame=0;
				}
				else
					now_frame++;
				m_lcd->clear_win_count();
				skip=skip_buf;
			}
			if (regs.LY>=144){ // VBlank 期間中 // During VBlank
				regs.STAT|=1;
				if (regs.LY==144){
					m_cpu->exec(72);
					m_cpu->irq(INT_VBLANK);
					if (regs.STAT&0x10)
						m_cpu->irq(INT_LCDC);
					m_cpu->exec(456-80);
				}
				else if (regs.LY==153){
					m_cpu->exec(80);
					regs.LY=0;
					// 前のラインのかなり早目から0になるようだ。
					// It's pretty early to be 0 from the previous line.
					m_cpu->exec(456-80);
					regs.LY=153;
				}
				else
					m_cpu->exec(456);
			}
			else{ // VBlank 期間外 // Period outside VBlank
				regs.STAT|=2;
				if (regs.STAT&0x20)
					m_cpu->irq(INT_LCDC);
				m_cpu->exec(80); // state=2
				regs.STAT|=3;
				m_cpu->exec(169); // state=3

				if (m_cpu->dma_executing){ // HBlank DMA
					if (m_cpu->b_dma_first){
						m_cpu->dma_dest_bank=m_cpu->vram_bank;
						if (m_cpu->dma_src<0x4000)
							m_cpu->dma_src_bank=m_rom->get_rom();
						else if (m_cpu->dma_src<0x8000)
							m_cpu->dma_src_bank=m_mbc->get_rom();
						else if (m_cpu->dma_src>=0xA000&&m_cpu->dma_src<0xC000)
							m_cpu->dma_src_bank=m_mbc->get_sram()-0xA000;
						else if (m_cpu->dma_src>=0xC000&&m_cpu->dma_src<0xD000)
							m_cpu->dma_src_bank=m_cpu->ram-0xC000;
						else if (m_cpu->dma_src>=0xD000&&m_cpu->dma_src<0xE000)
							m_cpu->dma_src_bank=m_cpu->ram_bank-0xD000;
						else m_cpu->dma_src_bank=NULL;
						m_cpu->b_dma_first=false;
					}
					memcpy(m_cpu->dma_dest_bank+(m_cpu->dma_dest&0x1ff0),m_cpu->dma_src_bank+m_cpu->dma_src,16);
//					fprintf(m_cpu->file,"%03d : dma exec %04X -> %04X rest %d\n",regs.LY,m_cpu->dma_src,m_cpu->dma_dest,m_cpu->dma_rest);

					m_cpu->dma_src+=16;
					m_cpu->dma_src&=0xfff0;
					m_cpu->dma_dest+=16;
					m_cpu->dma_dest&=0xfff0;
					m_cpu->dma_rest--;
					if (!m_cpu->dma_rest)
						m_cpu->dma_executing=false;

//					m_cpu->total_clock+=207*(m_cpu->speed?2:1);
//					m_cpu->sys_clock+=207*(m_cpu->speed?2:1);
//					m_cpu->div_clock+=207*(m_cpu->speed?2:1);
//					regs.STAT|=3;

					if (now_frame>=skip)
						m_lcd->render(vframe,regs.LY);

					regs.STAT&=0xfc;
					m_cpu->exec(207); // state=3
				}
				else{
/*					if (m_lcd->get_sprite_count()){
						if (m_lcd->get_sprite_count()>=10){
							m_cpu->exec(129);
							if ((regs.STAT&0x08))
								m_cpu->irq(INT_LCDC);
							regs.STAT&=0xfc;
							if (now_frame>=skip)
								m_lcd->render(vframe,regs.LY);
							m_cpu->exec(78); // state=0
						}
						else{
							m_cpu->exec(129*m_lcd->get_sprite_count()/10);
							if ((regs.STAT&0x08))
								m_cpu->irq(INT_LCDC);
							regs.STAT&=0xfc;
							if (now_frame>=skip)
								m_lcd->render(vframe,regs.LY);
							m_cpu->exec(207-(129*m_lcd->get_sprite_count()/10)); // state=0
						}
					}
					else{
*/						regs.STAT&=0xfc;
						if (now_frame>=skip)
							m_lcd->render(vframe,regs.LY);
						if ((regs.STAT&0x08))
							m_cpu->irq(INT_LCDC);
						m_cpu->exec(207); // state=0
//					}
				}
			}
		}
		else{ // LCDC 停止時 // LCDC is stopped
			regs.LY=0;
//			regs.LY=(regs.LY+1)%154;
			re_render++;
			if (re_render>=154){
				memset(vframe,0xff,160*144*2);
				m_renderer->refresh();
				if (now_frame>=skip){
					m_renderer->render_screen((byte*)vframe,160,144,16);
					now_frame=0;
				}
				else
					now_frame++;
				m_lcd->clear_win_count();
				re_render=0;
			}
			regs.STAT&=0xF8;
			m_cpu->exec(456);
		}
	}
}
