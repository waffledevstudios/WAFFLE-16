#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <SDL2/SDL.h>

#define ASSERT(_e, ...) if(!(_e)) {fprintf(stderr, __VA_ARGS__); exit(1); }
#define PASSERT(_e, ...) if(!(_e)) {fprintf(stderr, __VA_ARGS__); }

typedef float     f32;
typedef double    f64;
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint16_t  word;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef size_t    usize;

// Emulator Configs

	// Waffle-16 Configs

#define SCREENWIDTH 960
#define SCREENHEIGHT 540
#define RAMSIZE 1073741824 // 1 GiB
#define VRAMSIZE 536870912 // 1/2 GiB

	// Waffle-16++ Configs

#define SCREENWIDTHPLUS 1280
#define SCREENHEIGHTPLUS 720

// Math Data

#define PI 3.14159265359f
#define TAU (2.0f * PI)
#define PI_2 (PI / 2.0f)
#define PI_4 (PI / 4.0f)

#define v2_to_v2i(_v) ({ __typeof__(_v) __v = (_v); (v2i) { __v.x, __v.y }; })
#define v2i_to_v2(_v) ({ __typeof__(_v) __v = (_v); (v2) { __v.x, __v.y }; })

#define dot(_v0, _v1) ({ __typeof__(_v0) __v0 = (_v0), __v1 = (_v1); (__v0.x * __v1.x) + (__v0.y * __v1.y); })
#define length(_vl) ({ __typeof__(_vl) __vl = (_vl); sqrtf(dot(__vl, __vl)); })
#define normalize(_vn) ({ __typeof__(_vn) __vn = (_vn); const f32 l = length(__vn); (__typeof__(_vn)) { __vn.x / l, __vn.y / l }; })
#define min(_a, _b) ({ __typeof__(_a) __a = (_a), __b = (_b); __a < __b ? __a : __b; })
#define max(_a, _b) ({ __typeof__(_a) __a = (_a), __b = (_b); __a > __b ? __a : __b; })
#define clamp(_x, _mi, _ma) (min(max(_x, _mi), _ma))
#define ifnan(_x, _alt) ({ __typeof__(_x) __x = (_x); isnan(__x) ? (_alt) : __x; })
#define combine16string(a, b) ({return a + b * 65536;})

typedef struct {
    
	word GPRegisters[16];
	word Flags[32];
	word HDRWRBAR[16];
	u32 programCounter;
	word stack[sizeof(u16)];
	word stackPtr;
	u32 functionStack[sizeof(u8)];
	u8  functionStackPtr;
	word RAM[RAMSIZE/16];
	word VRAM[];

} emulationData_t;

typedef struct {
    
    u8 opcode;
	u16 p1;
	u16 p2;
	u8 p3;
    
} instruction_t;

typedef enum {
	
	Waffle16,
	Waffle16PlusPlus,

} emulatorVersion_t;

struct {

	SDL_Window* window;
	SDL_Texture* texture;
	SDL_Renderer* renderer;
	u32 pixels[SCREENWIDTHPLUS * SCREENHEIGHTPLUS];
	u16 frame;
	instruction_t instructions[65536];
    emulationData_t data;
	emulatorVersion_t emulatorVersion;
	bool quit;

} state;

void executeInstruction(){
    instruction_t instruction = state.instructions[state.data.programCounter];
	word* HDRWRBAR = state.data.HDRWRBAR;
	switch (instruction.opcode)
	{
		case 0x00:  // no operation
			break;

		case 0x01:  // move
			state.data.GPRegisters[instruction.p1] = instruction.p2;
			break;

		case 0x02:  // jump
			state.data.programCounter = instruction.p1 - 1;
			break;

		case 0x03:  // jump not zero
			if (state.data.GPRegisters[instruction.p1] != 0)
				state.data.programCounter = instruction.p1 - 1;
			break;

		case 0x04:  // increment
			state.data.GPRegisters[instruction.p1]++;
			break;

		case 0x05:  // decrement
			state.data.GPRegisters[instruction.p1]--;
			break;

		case 0x06:  // push
			state.data.stackPtr++;
			state.data.stack[state.data.stackPtr] = instruction.p1;
			break;
			
		case 0x07:  // push
			state.data.stackPtr++;
			state.data.stack[state.data.stackPtr] = state.data.GPRegisters[instruction.p1];
			break;

		case 0x08:  // pop
			state.data.GPRegisters[instruction.p1] = state.data.stack[state.data.stackPtr];
			state.data.stackPtr--;
			break;
			
		case 0x09:  // set stack pointer
			state.data.stackPtr = instruction.p1;
			break;

		case 0x0a:  // read ram
			if (state.data.HDRWRBAR[0xb] = 0) {
				if ((state.data.HDRWRBAR[0xc] + (state.data.HDRWRBAR[0xd] * 65536)) <= RAMSIZE)
					state.data.HDRWRBAR[0x3] = state.data.RAM[state.data.HDRWRBAR[0xc] + (state.data.HDRWRBAR[0xd] * 65536)]; }
			else if (state.data.HDRWRBAR[0xb] = 1); {
				if ((state.data.HDRWRBAR[0xc] + (state.data.HDRWRBAR[0xd] * 65536)) <= VRAMSIZE)
					state.data.HDRWRBAR[0x3] = state.data.VRAM[state.data.HDRWRBAR[0xc] + (state.data.HDRWRBAR[0xd] * 65536)]; }
			break;
		
		case 0x0b:  // write ram
			if (state.data.HDRWRBAR[0xb] = 0) {
				if ((state.data.HDRWRBAR[0xc] + (state.data.HDRWRBAR[0xd] * 65536)) <= RAMSIZE)
					state.data.RAM[state.data.HDRWRBAR[0xc] + (state.data.HDRWRBAR[0xd] * 65536)] = state.data.HDRWRBAR[0x3]; }
			else if (state.data.HDRWRBAR[0xb] = 1); {
					if ((state.data.HDRWRBAR[0xc] + (state.data.HDRWRBAR[0xd] * 65536)) <= VRAMSIZE)
						state.data.VRAM[state.data.HDRWRBAR[0xc] + (state.data.HDRWRBAR[0xd] * 65536)] = state.data.HDRWRBAR[0x3]; }
			break;

		case 0x0c:  // read hard drive
			FILE *sector;
			char* name = "HardDrive/sector" + state.data.HDRWRBAR[0xc] + (state.data.HDRWRBAR[0xd] * 65536);
			name = strcat(name, ".wds");
			sector = fopen(name, "rb");
			u16 buffer[65536];
			fread(buffer, 2, 65536, sector);
			fclose(sector);
			state.data.HDRWRBAR[0x3] = buffer[state.data.HDRWRBAR[0x5] + (state.data.HDRWRBAR[0x6] * 65536)];
			break;

		default:
			break;

		}
		state.data.programCounter++;
   
}

void run() {
    
	for (int i = 0; i < 20 ;i++) {
		executeInstruction();
	}   
}

void Init() {
	ASSERT(!SDL_Init(SDL_INIT_EVERYTHING),
		"SDL failed to Initialize: %s\n",
		SDL_GetError()
	);
	// Pick Waffle-16 or Waffle-16++
	printf("SDL Initialized Successfully\n");
	if (state.emulatorVersion = 1) {
		state.window = SDL_CreateWindow(
			"Waffle-16++ Emulator",                   // Window Title
			SDL_WINDOWPOS_CENTERED_DISPLAY(1),        // Window Position X
			SDL_WINDOWPOS_CENTERED_DISPLAY(1),        // Window Position Y
			SCREENWIDTHPLUS,                          // Window Width
			SCREENHEIGHTPLUS,                         // Window Height
			SDL_WINDOW_ALLOW_HIGHDPI                  // Window Flags
		);
	} else {
		state.window = SDL_CreateWindow(
			"Waffle-16 Emulator",                     // Window Title
			SDL_WINDOWPOS_CENTERED_DISPLAY(1),        // Window Position X
			SDL_WINDOWPOS_CENTERED_DISPLAY(1),        // Window Position Y
			SCREENWIDTH,                              // Window Width
			SCREENHEIGHT,                             // Window Height
			SDL_WINDOW_ALLOW_HIGHDPI                  // Window Flags
		);
	}

	ASSERT(state.window,
		"failed to create SDL window: %s\n",
		SDL_GetError()
	);
	printf("SDL Window Created Successfully\n");

	state.renderer = SDL_CreateRenderer(state.window, -1, SDL_RENDERER_PRESENTVSYNC);

	ASSERT(state.renderer,
		"failed to create SDL renderer: %s\n",
		SDL_GetError()
	);
	printf("SDL Renderer Created Successfully\n");

	state.texture =
		SDL_CreateTexture(
			state.renderer,
			SDL_PIXELFORMAT_ABGR8888,
			SDL_TEXTUREACCESS_STREAMING,
			SCREENWIDTH,
			SCREENHEIGHT
		);

	ASSERT(state.texture,
		"failed to create SDL texture: %s\n",
		SDL_GetError()
	);
}
int LoadBIOS() {
	FILE *BIOS;
	BIOS = fopen("System/bios.bin", "r");
	ASSERT(BIOS == NULL, "Failed to load BIOS.bin: %s\n", strerror(errno));
	printf("BIOS: %p\n");
	printf("Found BIOS\n");
	printf("Preparing BIOS\n");
	char* buffer;
	fgets(buffer, 64, BIOS);

	printf("Loading BIOS\n");

	if (BIOS != NULL) fclose(BIOS);
	printf("BIOS Load Completed\n");
	return 0;
}
int main(int argc, char **argv) {
	
	state.emulatorVersion = 1;

	Init();

	LoadBIOS();

	

	/* Run Loop
	state.frame = 0;
	while (!state.quit) {
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
				case SDL_QUIT:
					state.quit = true;
					break;
			
			}
		}
	
      run();        
        
		SDL_UpdateTexture(state.texture, NULL, state.pixels, SCREENWIDTH * 4);
		SDL_RenderCopyEx(
			state.renderer,
			state.texture,
			NULL,
			NULL,
			0.0,
			NULL,
			SDL_FLIP_VERTICAL);
		SDL_RenderPresent(state.renderer);
		state.frame++;
	}

	*/

	SDL_DestroyTexture(state.texture);
	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
	printf("SDL Quit Successfully\n");
	printf("Goodbye... :)\n");
	return 0;
}
