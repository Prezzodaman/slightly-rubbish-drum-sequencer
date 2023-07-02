#include <stdio.h>
#include <string.h>
#include <portmidi.h>
#include <stdbool.h>

#define MIDI_BUFFER_SIZE 32

#define MIDI_EVENT_NOTE 0x9
#define MIDI_EVENT_PITCH 0xe
#define MIDI_EVENT_CONTROL 0xb

#define MIDI_MODWHEEL 0x1
#define MIDI_BREATH 0x2
#define MIDI_VOLUME 0x7
#define MIDI_PROGRAM_CHANGE 0xc

#define MIDI_EVENT(value_1,value_2,channel,type) (channel | (type<<4) | (value_1<<8) | (value_2<<16))

#define DRUM_SOUNDS_MAX 32
#define DRUM_PATTERN_LENGTH 32
#define DRUM_CHANNEL 0
#define DRUM_ROOT_NOTE 60

#define PROG_CHANGE_ROOT_NOTE 48

typedef struct{
	char value_1;
	char value_2;
	char channel;
	char type;
} Event;

Event get_event(PmMessage message){
	Event event;
	event.value_1=(message & 0xff00) >> 8;
	event.value_2=(message & 0xff0000) >> 16;
	event.channel=message & 0xf;
	event.type=(message & 0xf0) >> 4;
	return event;
}

void note_on(PortMidiStream* stream,char note,char velocity,char channel){
	PmEvent buffer[1];
	buffer[0].message=MIDI_EVENT(note,velocity,channel,MIDI_EVENT_NOTE);
	Pm_Write(stream,buffer,1);
}

void note_off(PortMidiStream* stream,char note,char channel){
	PmEvent buffer[1];
	buffer[0].message=MIDI_EVENT(note,0,channel,MIDI_EVENT_NOTE);
	Pm_Write(stream,buffer,1);
}

void drum_trig(PortMidiStream* stream,char note,char velocity,char channel){
	PmEvent buffer[2];
	buffer[0].message=MIDI_EVENT(note,velocity,channel,MIDI_EVENT_NOTE);
	buffer[1].message=MIDI_EVENT(note,0,channel,MIDI_EVENT_NOTE);
	Pm_Write(stream,buffer,2);
}

void program_change(PortMidiStream* stream,char program){
	PmEvent buffer[1];
	buffer[0].message=MIDI_EVENT(program,0,0,MIDI_PROGRAM_CHANGE);
	Pm_Write(stream,buffer,1);
}

int main(){
	Pm_Initialize();

	int midi_in_device_id=3;
	int midi_out_device_id=2;
	int midi_open;

	bool degib=false;

	const PmDeviceInfo *midi_in_device_info;
	const PmDeviceInfo *midi_out_device_info;
	midi_in_device_info=Pm_GetDeviceInfo(0);
	midi_out_device_info=Pm_GetDeviceInfo(0);

	PortMidiStream *midi_in_stream;
	PortMidiStream *midi_out_stream;

	char input_string[]="Input";
	char output_string[]="Output";

	int bpm;
	int pattern_end;
	int drum_sounds;
	int prog_amount;	

	if(!degib){
		printf("MIDI output devices:\n");
		for(int a=0;a<Pm_CountDevices();a++){
			midi_out_device_info=Pm_GetDeviceInfo(a);
			if(midi_out_device_info->output){
				printf("%d: %s, %s\n",a,midi_out_device_info->name,output_string);
			}
		}
		printf("Choose an output device: ");
		while(!midi_out_device_info->output){
			scanf("%d",&midi_out_device_id);
			if(midi_out_device_id>=Pm_CountDevices()){
				printf("Device ID out of range! ");
			}else{
				midi_out_device_info=Pm_GetDeviceInfo(midi_out_device_id);
				if(!midi_out_device_info->output){
					printf("Not an output device! ");
				}
			}
		}
	}
	midi_open=Pm_OpenOutput(&midi_out_stream,midi_out_device_id,NULL,MIDI_BUFFER_SIZE,NULL,NULL,0);
	if(midi_open==0){	
		printf("MIDI out opened successfully!\n");
	}else{
		printf("Failed to open MIDI out :(\n");
		return 0;
	}

	if(!degib){
		printf("\nMIDI input devices:\n");
		for(int a=0;a<Pm_CountDevices();a++){
			midi_out_device_info=Pm_GetDeviceInfo(a); // can be either in or out, we only need the same info for both menus
			if(midi_out_device_info->input){
				printf("%d: %s, %s\n",a,midi_out_device_info->name,input_string);
			}
		}
		printf("Choose an input device: ");
		scanf("%d",&midi_in_device_id);
	}
	
	midi_open=Pm_OpenInput(&midi_in_stream,midi_in_device_id,NULL,MIDI_BUFFER_SIZE,NULL,NULL);
	if(midi_open==0){
		printf("MIDI in opened successfully!\n");
	}else{
		printf("Failed to open MIDI in :(\n");
		return 0;
	}

	if(degib){
		pattern_end=16;
		drum_sounds=10;
		bpm=120;
		prog_amount=3;
	}else{
		printf("\nEnter BPM: ");
		scanf("%d",&bpm);
		printf("Enter pattern length: ");
		while(pattern_end>DRUM_PATTERN_LENGTH || pattern_end<8){
			scanf("%d",&pattern_end);
			if(pattern_end>DRUM_PATTERN_LENGTH || pattern_end<8){
				printf("Pattern must between 8 and %d steps! ",DRUM_PATTERN_LENGTH);
			}
		}
		printf("Enter amount of drum sounds (from root note %d): ",DRUM_ROOT_NOTE);
		while(drum_sounds>DRUM_SOUNDS_MAX || drum_sounds<8){
			scanf("%d",&drum_sounds);
			if(drum_sounds>DRUM_SOUNDS_MAX || drum_sounds<8){
				printf("Maximum of %d drum sounds! ",DRUM_SOUNDS_MAX);
			}
		}
	}
	//

	char drum_pattern[DRUM_SOUNDS_MAX][DRUM_PATTERN_LENGTH];
	int drum=0;
	int step=0;
	int program=0;
	int bpm_ms=(60000/bpm)/4;
	bool erasing=false;
	int erasing_drum=drum_sounds;

	for(drum=0;drum<DRUM_SOUNDS_MAX;drum++){
		for(step=0;step<DRUM_PATTERN_LENGTH;step++){
			drum_pattern[drum][step]=0;
		}
	}	

	for(step=0;step<pattern_end;step++){
		if(step%4==0){
			drum_pattern[0][step]=1;
		}
		/*if(step%2==0){
			drum_pattern[7][step]=1;
		}
		if((step+4)%8==0){
			drum_pattern[1][step]=1;
		}*/
	}

	// amen!
	/*drum_pattern[0][0]=1;
	drum_pattern[1][2]=1;
	drum_pattern[2][4]=1;
	drum_pattern[3][6]=1;
	drum_pattern[4][7]=1;
	drum_pattern[5][8]=1;
	drum_pattern[6][9]=1;
	drum_pattern[7][10]=1;
	drum_pattern[8][11]=1;
	drum_pattern[9][12]=1;
	drum_pattern[10][14]=1;
	drum_pattern[11][15]=1;*/

	step=0;

	//

	PmEvent midi_in_buffer[1];
	PmEvent midi_out_buffer[1];
	PmEvent midi_clock_buffer[24];
	Event midi_in_event;

	midi_out_buffer[0].message=0xfa;
	Pm_Write(midi_out_stream,midi_out_buffer,1);

	for(int a=0;a<24;a++){
		midi_clock_buffer[a].message=0xf8;
	}

	while(1){
		printf("\e[H\e[2J\e[3J");

		int punch_step_number=step-1;
		int punch_drum_number;
		if(punch_step_number<0){
			punch_step_number=pattern_end-1;
		}
		if(erasing && midi_in_event.value_1>=DRUM_ROOT_NOTE && midi_in_event.value_1<DRUM_ROOT_NOTE+drum_sounds && midi_in_event.value_2>0){
			drum_pattern[punch_drum_number][punch_step_number]=0;
		}
		if(Pm_Poll(midi_in_stream)){
			for(int a=0;a<2;a++){ // gets both the note on/note offs
				bool drum_hit=false;
				Pm_Read(midi_in_stream,midi_in_buffer,1);
				midi_in_event=get_event(midi_in_buffer[0].message);

				if(midi_in_event.type==MIDI_EVENT_NOTE){
					punch_drum_number=midi_in_event.value_1-DRUM_ROOT_NOTE;
					if(midi_in_event.value_2>0 && !drum_hit){
						if(midi_in_event.value_1>=DRUM_ROOT_NOTE && midi_in_event.value_1<DRUM_ROOT_NOTE+drum_sounds){
							if(!erasing){
								if(drum_pattern[punch_drum_number][punch_step_number]^=1){
									drum_trig(midi_out_stream,midi_in_event.value_1,127,DRUM_CHANNEL);
								}
							}
							erasing=true;
							erasing_drum=punch_drum_number;
							drum_hit=true;
						}else if(midi_in_event.value_1>=PROG_CHANGE_ROOT_NOTE && midi_in_event.value_1<PROG_CHANGE_ROOT_NOTE+prog_amount){
							program=midi_in_event.value_1-PROG_CHANGE_ROOT_NOTE;
							program_change(midi_out_stream,program);
						}
					}else{
						erasing=false;
						erasing_drum=drum_sounds;
					}
				}
			}
		}

		for(drum=0;drum<drum_sounds;drum++){
			printf("Drum %2d:",drum+1);
			for(int a=0;a<pattern_end;a++){
				if(drum_pattern[drum][a]){
					printf("*");
				}else{
					printf(" ");
				}
			}
			printf("\n");
		}
		printf("       ");
		if(drum_sounds>9){
			printf(" ");
		}
		if(step>0){
			printf("%*s",step," ");
		}
		printf("^\n");
		printf("BPM: %d, Program: %d, Length: %d beats\n",bpm,program+1,pattern_end);

		Pt_Sleep(bpm_ms);

		if(step%2==0){
			Pm_Write(midi_out_stream,midi_clock_buffer,24);
		}

		for(drum=0;drum<drum_sounds;drum++){
			if(drum_pattern[drum][step]>0){
				if(erasing){
					if(drum!=erasing_drum){
						drum_trig(midi_out_stream,DRUM_ROOT_NOTE+drum,127,DRUM_CHANNEL);
					}
				}else{
					drum_trig(midi_out_stream,DRUM_ROOT_NOTE+drum,127,DRUM_CHANNEL);
				}
			}
		}	

		if(step<pattern_end-1){
			step++;
		}else{
			step=0;
		}
	}

	Pm_Terminate();
}