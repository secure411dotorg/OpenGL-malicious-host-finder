/*
    Copyright (C) 2009 Andrew Caudwell (acaudwell@gmail.com)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "main.h"

int main(int argc, char *argv[]) {

    SDLAppInit("Mal4s", "mal4s");

#ifdef _WIN32
        SDLApp::initConsole();
#endif

    ConfFile conf;
    ConfFile textConf;
    std::vector<std::string> files;
    std::string textConfFile;
    std::string demoFile = texturemanager.getDir() + "sample--newns.mal4s";
    int demoindex = 2;
    char* demo[2];
    demo[0] = argv[0];
    demo[1] = strdup(demoFile.c_str());

    //convert args to a conf file
    //read the conf file
    //apply the conf file to settings
    try {
        if(argc == 1) {
		printf("No arguments supplied, using sample file: %s.\n", demoFile.c_str());
		gGourceSettings.parseArgs(demoindex, demo, conf, &files);
	} else {
	        gGourceSettings.parseArgs(argc, argv, conf, &files);
	}
        Logger::getDefault()->setLevel(gGourceSettings.log_level);

	if(!files.empty() && gGourceSettings.load_text_config.empty()) {
		size_t conf_marker = files[0].find_last_of("--");
		if(conf_marker != std::string::npos) {
			size_t conf_end_marker = files[0].find_last_of(".");
			if(conf_end_marker != std::string::npos) {
				textConfFile = files[0].substr(conf_marker + 1, conf_end_marker - conf_marker - 1) + ".conf";
				if(!boost::filesystem::exists(textConfFile.c_str())) {
					if(!gGourceSettings.textConfDir.empty()) {
						std::string testforconfig = gGourceSettings.textConfDir + textConfFile;
						if(boost::filesystem::exists(testforconfig.c_str())) {
							textConfFile = testforconfig;
						} else {
							textConfFile = texturemanager.getDir() + textConfFile;
							if(!boost::filesystem::exists(textConfFile.c_str())) {
								textConfFile.clear();
							}
						}
					} else {
						textConfFile = texturemanager.getDir() + textConfFile;
							if(!boost::filesystem::exists(textConfFile.c_str())) {
								textConfFile.clear();
							}
					}
				}
			}
		}
	}
	if(!gGourceSettings.load_text_config.empty()) textConfFile = gGourceSettings.load_text_config;

	//apply text formatting
	if(!textConfFile.empty()) {
		fprintf(stdout, "Using text config from: %s.\n", textConfFile.c_str());
		textConf.load(textConfFile);
		gGourceSettings.parseArgs(argc, argv, textConf);
	}

	//Test if dissect.conf exists in the working directory and make it load as the default config file.
	if(gGourceSettings.load_config.empty() && boost::filesystem::exists("dissect.conf")) {
		gGourceSettings.load_config = "dissect.conf";
	}

        if(gGourceSettings.load_config.empty() && !files.empty()) {
            //see if file looks like a config file
            for(std::vector<std::string>::iterator fit = files.begin(); fit != files.end(); fit++) {
                std::string file = *fit;

                int file_length = file.size();

                if(   (file.rfind(".conf") == (file_length-5) && file_length > 5)
                   || (file.rfind(".cfg")  == (file_length-4) && file_length > 4)
                   || (file.rfind(".ini")  == (file_length-4) && file_length > 4) ) {

                    bool is_conf=true;

                    try {
                        ConfFile conftest;
                        conftest.load(file);
                    } catch(ConfFileException& exception) {
                        is_conf = false;
                    }

                    if(is_conf) {
                        gGourceSettings.load_config = file;
                        files.erase(fit);
                        break;
                    }
                }
            }
        }

        //set log level
        Logger::getDefault()->setLevel(gGourceSettings.log_level);

#ifdef _WIN32
        // hide console if not needed
        if(gGourceSettings.log_level == LOG_LEVEL_OFF && !SDLApp::existing_console) {
            SDLApp::showConsole(false);
        }
#endif

        //load config
        if(!gGourceSettings.load_config.empty()) {
            conf.clear();
            conf.load(gGourceSettings.load_config);


            //apply args to loaded conf file
            gGourceSettings.parseArgs(argc, argv, conf);
        }

        //set path
        if(!files.empty()) {
            std::string path = files[files.size()-1];

            ConfSectionList* sectionlist = conf.getSections("gource");

            if(sectionlist!=0) {
                for(ConfSectionList::iterator sit = sectionlist->begin(); sit != sectionlist->end(); sit++) {
                    (*sit)->setEntry("path", path);
                }
            } else {
                conf.setEntry("gource", "path", path);
            }
        }

        //apply the config / see if its valid
        gGourceSettings.importDisplaySettings(conf);
        gGourceSettings.importGourceSettings(conf);

	if(!textConfFile.empty()) {
	        gGourceSettings.importTextSettings(textConf);
	} else gGourceSettings.importTextSettings(conf);

        //save config
        if(!gGourceSettings.save_config.empty()) {
            conf.save(gGourceSettings.save_config);
            exit(0);
        }

        //write custom log file
        if(!gGourceSettings.output_custom_filename.empty() && !gGourceSettings.path.empty()) {

            Gource::writeCustomLog(gGourceSettings.path, gGourceSettings.output_custom_filename);
            exit(0);
        }

    } catch(ConfFileException& exception) {

        SDLAppQuit(exception.what());
    }

    // this causes corruption on some video drivers
    if(gGourceSettings.multisample) {
        display.multiSample(4);
    }

    //background needs alpha channel
    if(gGourceSettings.transparent) {
        display.enableAlpha(true);
    }

    //enable vsync
    display.enableVsync(gGourceSettings.vsync);

    //allow resizing window if we are not recording
    if(gGourceSettings.resizable && gGourceSettings.output_ppm_filename.empty()) {
        display.enableResize(true);
    }

    try {

        display.init("Mal4s", gGourceSettings.display_width, gGourceSettings.display_height, gGourceSettings.fullscreen);

#if SDL_VERSION_ATLEAST(2,0,0)
        SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
#endif
        
    } catch(SDLInitException& exception) {

        char errormsg[1024];
        snprintf(errormsg, 1024, "SDL initialization failed - %s", exception.what());

        SDLAppQuit(errormsg);
    }

    //Open sound
    Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 );
    Mix_Music *music = NULL;
    Mix_PlayMusic(music, 1);

    //init frame exporter
    FrameExporter* exporter = 0;

    if(gGourceSettings.output_ppm_filename.size() > 0) {

        try {

            exporter = new PPMExporter(gGourceSettings.output_ppm_filename);

        } catch(PPMExporterException& exception) {

            char errormsg[1024];
            snprintf(errormsg, 1024, "could not write to '%s'", exception.what());

            SDLAppQuit(errormsg);
        }
    }

    if(display.multiSamplingEnabled()) {
        glEnable(GL_MULTISAMPLE_ARB);
    }

    GourceShell* gourcesh = 0;

    try {
        gourcesh = gGourceShell = new GourceShell(&conf, exporter);
        gourcesh->run();

    } catch(ResourceException& exception) {

        char errormsg[1024];
        snprintf(errormsg, 1024, "failed to load resource '%s'", exception.what());

        SDLAppQuit(errormsg);

    } catch(SDLAppException& exception) {

        if(exception.showHelp()) {
            gGourceSettings.help();
        } else {
            SDLAppQuit(exception.what());
        }
    }

    gGourceShell = 0;

    if(gourcesh != 0) delete gourcesh;
    if(exporter != 0) delete exporter;

    //free resources
    display.quit();

    return 0;
}
