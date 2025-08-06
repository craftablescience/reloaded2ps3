// ReSharper disable CppRedundantQualifier
// ReSharper disable CppUseStructuredBinding

#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <argparse/argparse.hpp>
#include <sourcepp/FS.h>
#include <sourcepp/String.h>
#include <steampp/steampp.h>
#include <vpkpp/vpkpp.h>

#include "Config.h"
#include "MainMenu.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

namespace {

void openUrl(std::string_view url) {
#if defined(_WIN32)
	ShellExecuteA(nullptr, nullptr, url.data(), nullptr, nullptr, SW_SHOW);
#elif defined(__APPLE__)
	system(("open " + std::string{url}).c_str());
#else
	system(("xdg-open " + std::string{url}).c_str());
#endif
}

int runExecutable(std::string_view commandWindows, std::string_view commandOther, std::string args) {
#ifdef _WIN32
	sourcepp::string::denormalizeSlashes(args, false, false);
	STARTUPINFOA si{};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};
	if (!CreateProcessA(commandWindows.data(), args.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
		return 1;
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD ec;
	GetExitCodeProcess(pi.hProcess, &ec);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return static_cast<int>(ec);
#else
	sourcepp::string::normalizeSlashes(args, false, false);
	return system((std::string{commandOther} + ' ' + args).c_str());
#endif
}

int runExecutable(std::string_view command, std::string args) {
	std::string commandWindows{command};
	sourcepp::string::denormalizeSlashes(commandWindows, false, false);
	std::string commandOther{command};
	sourcepp::string::normalizeSlashes(commandOther, false, false);
	if (command.ends_with(".exe")) {
		return ::runExecutable(commandWindows, "wineconsole " + std::string{commandOther}, std::move(args));
	}
	return ::runExecutable(commandWindows, commandOther, std::move(args));
}

} // namespace

int main(int argc, const char* const argv[]) {
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8); // Set up console to show UTF-8 characters
	setvbuf(stdout, nullptr, _IOFBF, 1000); // Enable buffering so VS terminal won't chop up UTF-8 byte sequences
#endif

	argparse::ArgumentParser cli{PROJECT_NAME, PROJECT_VERSION, argparse::default_arguments::help};

#ifdef _WIN32
	// Add the Windows-specific ones because why not
	cli.set_prefix_chars("-/");
	cli.set_assign_chars("=:");
#endif

	std::string output;
	cli
		.add_argument("-o", "--output")
		.metavar("PATH")
		.help("The parent directory of the generated xlsppatch directory.")
		.store_into(output);

	bool debugFormat;
	cli
		.add_argument("-d", "--debug")
		.help("Make the produced zip file readable by common zip programs. Should still be loadable by the PS3.")
		.flag()
		.store_into(debugFormat);

	cli.add_epilog("Program details:\n\n"
		PROJECT_NAME_PRETTY " — version v" PROJECT_VERSION " — created by " PROJECT_ORGANIZATION_NAME " — licensed under MIT\n\n"
		"Want to report a bug or request a feature? Make an issue at " PROJECT_HOMEPAGE_URL "/issues");

	try {
		cli.parse_args(argc, argv);

		// Step 0 - Output location

		std::filesystem::path outputPath = output;
		if (output.empty()) {
			outputPath = "xlsppatch";
		} else {
			outputPath /= "xlsppatch";
		}
		if (std::filesystem::exists(outputPath)) {
			std::filesystem::remove_all(outputPath);
		}
		outputPath = std::filesystem::absolute(outputPath);
		std::filesystem::create_directory(outputPath);
		std::cout << "Using output directory: " << outputPath << std::endl;

		outputPath = outputPath / "game";
		std::filesystem::create_directory(outputPath);

		// Step 1 - Find paths

		std::filesystem::path dinoDDayBase, portalReloadedBase;
		steampp::Steam steam;
		static constexpr steampp::AppID dinoDDaySDKAppID = 70004;
		static constexpr steampp::AppID portalReloadedAppID = 1255980;

		if (!steam.isAppInstalled(dinoDDaySDKAppID)) {
			std::cout << "Dino D-Day SDK could not be found!" << std::endl;
			std::cout << "Enter the path to the Dino D-Day SDK now, or press Enter to exit: ";
			std::string dinoDDayBaseStr;
			std::getline(std::cin, dinoDDayBaseStr);
			dinoDDayBase = dinoDDayBaseStr;
		} else {
			dinoDDayBase = steam.getAppInstallDir(dinoDDaySDKAppID);
		}
		if (dinoDDayBase.empty()) {
			::openUrl(std::format("steam://install/{}", dinoDDaySDKAppID));
			return EXIT_FAILURE;
		}
		std::cout << "Found Dino D-Day SDK location at " << dinoDDayBase << std::endl;

		if (!steam.isAppInstalled(portalReloadedAppID)) {
			std::cout << "Portal Reloaded could not be found!" << std::endl;
			std::cout << "Enter the path to Portal Reloaded now, or press Enter to exit: ";
			std::string portalReloadedBaseStr;
			std::getline(std::cin, portalReloadedBaseStr);
			portalReloadedBase = portalReloadedBaseStr;
		} else {
			portalReloadedBase = steam.getAppInstallDir(portalReloadedAppID);
		}
		if (portalReloadedBase.empty()) {
			::openUrl(std::format("steam://install/{}", portalReloadedAppID));
			return EXIT_FAILURE;
		}
		std::cout << "Found Portal Reloaded location at " << portalReloadedBase << std::endl;

		// Step 2 - Copy some files

		std::cout << "Copying game files to output directory..." << std::endl;

		std::filesystem::copy(dinoDDayBase / "bin", outputPath / "bin", std::filesystem::copy_options::update_existing | std::filesystem::copy_options::recursive);
		std::filesystem::copy(portalReloadedBase / "portalreloaded", outputPath / "portalreloaded", std::filesystem::copy_options::update_existing | std::filesystem::copy_options::recursive);
		if (std::filesystem::exists(outputPath / "portalreloaded" / "SAVE")) {
			std::filesystem::remove_all(outputPath / "portalreloaded" / "SAVE");
		}
		if (std::filesystem::exists(outputPath / "portalreloaded" / "steam.inf")) {
			std::filesystem::remove(outputPath / "portalreloaded" / "steam.inf");
		}
		if (std::filesystem::exists(outputPath / "portalreloaded" / "workshop_log.txt")) {
			std::filesystem::remove(outputPath / "portalreloaded" / "workshop_log.txt");
		}

		// Step 3 - Extract the VPK

		const auto vpk = vpkpp::VPK::open((outputPath / "portalreloaded" / "pak01_dir.vpk").string());
		if (!vpk) {
			std::cout << "Failed to load portalreloaded VPK!" << std::endl;
			return EXIT_FAILURE;
		}
		std::cout << "Extracting contents of portalreloaded VPK..." << std::endl;
		if (!vpk->extractAll((outputPath / "portalreloaded").string(), false)) {
			std::cout << "Failed to extract portalreloaded VPK!" << std::endl;
			return EXIT_FAILURE;
		}
		std::filesystem::remove(outputPath / "portalreloaded" / "pak01_dir.vpk");

		// Step 4 - Pull in some required assets

		const auto vpk2 = vpkpp::VPK::open((portalReloadedBase / "portal2_dlc2" / "pak01_dir.vpk").string());
		if (!vpk2) {
			std::cout << "Failed to load portal2_dlc2 VPK!" << std::endl;
			return EXIT_FAILURE;
		}
		std::cout << "Extracting specific files from portal2_dlc2 VPK..." << std::endl;
		if (!vpk2->extractDirectory("models",  (outputPath / "portalreloaded" / "models").string())) {
			std::cout << "Failed to extract models directory from portal2_dlc2 VPK!" << std::endl;
			return EXIT_FAILURE;
		}
		if (!vpk2->extractDirectory("materials/models",  (outputPath / "portalreloaded" / "materials" / "models").string())) {
			std::cout << "Failed to extract materials/models directory from portal2_dlc2 VPK!" << std::endl;
			return EXIT_FAILURE;
		}
		if (!vpk2->extractDirectory("materials/tile",  (outputPath / "portalreloaded" / "materials" / "tile").string())) {
			std::cout << "Failed to extract materials/tile directory from portal2_dlc2 VPK!" << std::endl;
			return EXIT_FAILURE;
		}

		// Step 5 - Fix up vtx file extensions

		std::vector<std::filesystem::path> vtxFiles;
		for (const auto& file : std::filesystem::recursive_directory_iterator{outputPath / "portalreloaded" / "models"}) {
			if (auto path = file.path(); file.is_regular_file() && path.string().ends_with(".vtx") && !path.string().ends_with(".dx80.vtx") && !path.string().ends_with(".dx90.vtx") && !path.string().ends_with(".sw.vtx")) {
				path.replace_extension(".dx90.vtx");
				if (!std::filesystem::exists(path)) {
					vtxFiles.push_back(file.path());
				}
			}
		}
		for (const auto& path : vtxFiles) {
			std::cout << "Appending dx90 suffix to " << path << "..." << std::endl;
			auto pathNew{path};
			pathNew.replace_extension(".dx90.vtx");
			std::filesystem::rename(path, pathNew);
		}

		// Step 6 - Fix the main menu

		std::cout << "Fixing main menu..." << std::endl;
		const auto baseModUIFolder = outputPath / "portalreloaded" / "resource" / "ui" / "basemodui";
		std::filesystem::remove_all(baseModUIFolder);
		std::filesystem::create_directory(baseModUIFolder);
		sourcepp::fs::writeFileText((baseModUIFolder / "mainmenu.res").string(), std::string{g_mainmenu_res});

		std::cout << "Appending ps3 suffix to startupvids.txt..." << std::endl;
		std::filesystem::rename(outputPath / "portalreloaded" / "media" / "startupvids.txt", outputPath / "portalreloaded" / "media" / "startupvids.ps3.txt");

		// Step 7 - Run makegamedata

		std::cout << "Making game zip. This will take a while..." << std::endl;
		std::filesystem::current_path(outputPath / "portalreloaded");
		std::string args = "-ps3 -r -z ../../zip0.ps3.zip";
		if (debugFormat) {
			args += " -zipformat";
		}
		if (!::runExecutable("../bin/makegamedata.exe", args)) {
			std::cout << "Failed to make game data!" << std::endl;
			return EXIT_FAILURE;
		}
		std::filesystem::current_path(outputPath / "..");
		std::cout << std::endl;

		// Step 8 - Fix up maps and media files

		std::filesystem::create_directory(outputPath / ".." / "maps");
		for (const auto& map : std::filesystem::recursive_directory_iterator{outputPath / "portalreloaded" / "maps"}) {
			if (map.path().string().ends_with(".ps3.bsp")) {
				std::cout << "Copying " << map.path().filename() << " to output maps directory..." << std::endl;
				std::filesystem::rename(map.path(), outputPath / ".." / "maps" / map.path().filename());
			}
		}
		std::filesystem::create_directory(outputPath / ".." / "media");
		for (const auto& media : std::filesystem::recursive_directory_iterator{outputPath / "portalreloaded" / "media"}) {
			if (media.path().string().ends_with(".bik")) {
				std::cout << "Copying " << media.path().filename() << " to output media directory..." << std::endl;
				std::filesystem::rename(media.path(), outputPath / ".." / "media" / media.path().filename());
			}
		}

		std::cout << "Removing temporary files..." << std::endl;
		std::filesystem::remove_all(outputPath);

		std::cout << "Complete!" << std::endl;

	} catch (const std::exception& e) {
		std::cerr << e.what() << '\n' << std::endl;
		std::cerr << "Run " << argv[0] << " with no arguments for usage information." << '\n' << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
