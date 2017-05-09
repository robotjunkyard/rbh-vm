// ImGui - standalone example application for SDL2 + OpenGL
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "RobotVM.h"
#include "VMAssembler.h"
#include "TextBuffer.h"

#include <vector>
#include <map>
#include <stdio.h>

#pragma warning( push, 0 )
#include <SDL.h>
#include <SDL_opengl.h>
#pragma warning( pop ) 


#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))
#define IM_MAX(_A,_B)       (((_A) >= (_B)) ? (_A) : (_B))

// yeah yeah yeah, I'll move these globals into neat organized classes later on, after this project feels less like a crude prototype
int resx = 1920, resy = 1080;
std::vector<TextBuffer<8192>> codeSamples;
int currentSampleIdx = 1;

bool show_imgui_test_window = true;

void initCodeSamples()
{
	// there's no way to get good-looking indentation here, so whatever
	codeSamples.push_back(
		// very simple Countdown loop
		{ std::string{
				"start: MOV R1,40\n"
				"loop:  ADD R1,-1\n"
				"       JNZERO R1,loop\n"
				// "       MUL R1,R2   \n"
				// "       JNEG R1,derp\n"
				"       HALT \n" },
			"Countdown",
			"A simple countdown loop, with R1 going from 40 down to 0" } );

	codeSamples.push_back(
		{ std::string{
				"start: MOV R4,8\n"          // how many bytes
				"       MOV R1,2\n"          // initial value to write
				"       MOV R3,200\n"        // begin write pointer to RAM
				"loop:  JZERO R4,end\n"      // terminate when finished
				"       MOVRP R3,R1\n"       // byte mem[r3] = truncated word R1
				"       ADD R3,1\n"          // inc write pointer
				"       ADD R4,-1\n"         // dec loop counter
				"       MUL R1,2\n"          // r1 *= 2
				"       JMP loop\n"
				"end:   DUP R1\n"            // set all registers to same val as R1
				"       HALT\n" },
			"WriteLoop"
			"Writes n*=2 to contiguous memory 'R4' times" } );
}

bool renderGUI(SDL_Window* window, VMAssembler *asmblr, RobotVM *vm);
bool drawGUI = true;

#ifdef WIN32
int WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char *argv[])
#endif
{
	initCodeSamples();
    std::shared_ptr<RobotVM> vmp(new RobotVM());

    RobotVM&         vm = *vmp;
    VMInstrEmitter&   e = vm.emitter();
    VMAssembler asmblr( (std::shared_ptr<RobotVM>(vmp)), e );

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_DisplayMode desktopDisplaySettings;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_GetDesktopDisplayMode(0, &desktopDisplaySettings);

    SDL_Window *window = SDL_CreateWindow("robots for hire", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, resx, resy,
            SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_RESIZABLE);
    SDL_GLContext glcontext = SDL_GL_CreateContext(window);

    SDL_SetWindowGrab(window, SDL_FALSE);

    SDL_DisplayMode* wdispmode = nullptr;
    SDL_GetWindowDisplayMode(window, wdispmode);

    // init ImGui
    ImGui_ImplSdl_Init(window);

    // Load Fonts
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("fonts/mplus-1mn-medium.ttf", 18.0f, NULL /*, io.Fonts->GetGlyphRangesJapanese() */);

    ImVec4 clear_color = ImColor(114, 144, 154);

    // Main loop
    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
			if (drawGUI) ImGui_ImplSdl_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
			else if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.scancode)
				{
				case SDL_SCANCODE_F11:  drawGUI = !drawGUI;
				default: break;
				}
			}
        }

		if (drawGUI && !done)
			done = renderGUI(window, &asmblr, &vm);

        // Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_imgui_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_imgui_test_window);
        }

        // Rendering
        //glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glViewport(0, 0, resx, resy);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

		if (drawGUI) ImGui::Render();
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplSdl_Shutdown();
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

bool renderGUI(SDL_Window* window, VMAssembler *asmblr, RobotVM *vm)
{
	bool done = false;
	static std::vector<word_t> regHistory[5];  // populated by STEP
	static std::string romdump = "";

	ImGui_ImplSdl_NewFrame(window);

	ImVec2 desktopWindowPos(0.0, 0.0);
	ImVec2 desktopWindowSize((resx / 2) + 48, resy);
	ImVec2 machineWindowPos((resx / 2) + 48, 0);
	ImVec2 machineWindowSize(resx - ((resx / 2) + 48), resy);
	ImGuiWindowFlags desktopWindowFlags = 0;
	static bool showWindows = true;
	desktopWindowFlags |= ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
	| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ShowBorders;

	// 1. Show a simple window
	// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
	{
		ImGui::SetNextWindowPos(desktopWindowPos);
		ImGui::SetNextWindowSize(desktopWindowSize);

		ImGui::Begin("Robots For Hire (VM Dashboard)", &showWindows, desktopWindowFlags);
		ImGui::Text("Code:");

		// ImGui::Combo("Combo", &currentSampleIdx,

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				//if (ImGui::MenuItem("Countdown From 40")) {}
				//if (ImGui::MenuItem("(TODO) Swap Elements Across Arrays")) {}
				//if (ImGui::MenuItem("(TODO) Reverse Array")) {}
				if (ImGui::MenuItem("Quit")) { done = true; }
				ImGui::EndMenu();
			}

			/*if (ImGui::BeginMenu("Edit"))
			{
				ImGui::MenuItem("Test 1", NULL);
				ImGui::MenuItem("Test 2", NULL, &testToggleValue);
				ImGui::EndMenu();
			}*/

			ImGui::EndMenuBar();
		}

		static bool read_only = false;

		ImGui::InputTextMultiline("##code", (codeSamples[currentSampleIdx]).text(), codeSamples[currentSampleIdx].length(), ImVec2(256, ImGui::GetTextLineHeight() * 16),
			ImGuiInputTextFlags_AllowTabInput | (read_only ? ImGuiInputTextFlags_ReadOnly : 0));
		ImGui::SameLine();
		// TODO: is there some widget in ImGui that does this -- bordered text -- but without presuming text input?
		ImGui::InputTextMultiline("##romdump", const_cast<char*>(romdump.c_str()), romdump.length(), ImVec2(-1, ImGui::GetTextLineHeight() * 16),
			ImGuiInputTextFlags_ReadOnly);

		if (ImGui::Button("Compile"))
		{
			vm->reset();
			asmblr->reset();

			asmblr->parsetextblock(codeSamples[currentSampleIdx].text());
			asmblr->walkFirstPass();
			asmblr->walkSecondPass();
			romdump = vm->printROMToString();
		}

		ImGui::SameLine();
		if (ImGui::Button("Step"))
		{
			if (!vm->isHalted())
			{
				vm->step();

				regHistory[0].push_back(vm->getRegs().pc);
				regHistory[1].push_back(vm->getRegs().r1);
				regHistory[2].push_back(vm->getRegs().r2);
				regHistory[3].push_back(vm->getRegs().r3);
				regHistory[4].push_back(vm->getRegs().r4);
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Step x5"))
		{
			for (int i = 0; i < 5; i++)
			{
				if (!vm->isHalted())
				{
					vm->step();

					regHistory[0].push_back(vm->getRegs().pc);
					regHistory[1].push_back(vm->getRegs().r1);
					regHistory[2].push_back(vm->getRegs().r2);
					regHistory[3].push_back(vm->getRegs().r3);
					regHistory[4].push_back(vm->getRegs().r4);
				}
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Step x10"))
		{
			for (int i = 0; i < 10; i++)
			{
				if (!vm->isHalted())
				{
					vm->step();

					regHistory[0].push_back(vm->getRegs().pc);
					regHistory[1].push_back(vm->getRegs().r1);
					regHistory[2].push_back(vm->getRegs().r2);
					regHistory[3].push_back(vm->getRegs().r3);
					regHistory[4].push_back(vm->getRegs().r4);
				}
			}
		}

		ImGui::BeginChild("Table", ImVec2(0, -1), true);
		ImGui::Text("Registers");

		ImGui::Columns(5, "regcolumns"); // 4-ways, with border
		ImGui::Separator();
		ImGui::Text("pc"); ImGui::NextColumn();
		ImGui::Text("r1"); ImGui::NextColumn();
		ImGui::Text("r2"); ImGui::NextColumn();
		ImGui::Text("r3"); ImGui::NextColumn();
		ImGui::Text("r4"); ImGui::NextColumn();
		ImGui::Separator();

		for (int i = 0; i < regHistory[0].size(); i++)
		{
			//ImGui::NextColumn();
			ImGui::Text("%04x", regHistory[0].at(i));
			ImGui::NextColumn();
			ImGui::Text("%04x (D:%04d)", regHistory[1].at(i), regHistory[1].at(i));
			ImGui::NextColumn();
			ImGui::Text("%04x (D:%04d)", regHistory[2].at(i), regHistory[2].at(i));
			ImGui::NextColumn();
			ImGui::Text("%04x (D:%04d)", regHistory[3].at(i), regHistory[3].at(i));
			ImGui::NextColumn();
			ImGui::Text("%04x (D:%04d)", regHistory[4].at(i), regHistory[4].at(i));
			ImGui::NextColumn();
		}

		if (vm->isHalted())
		{
			ImGui::Text("HALTED!");
			ImGui::NextColumn();
		}

		ImGui::EndChild();

		//ImGui::Text(romdump.c_str());
		ImGui::End(); // window
	}

	return done;
}

