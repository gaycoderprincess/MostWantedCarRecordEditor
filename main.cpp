#include <windows.h>
#include <mutex>
#include <filesystem>
#include <format>
#include <toml++/toml.hpp>

#include "nya_commonhooklib.h"
#include "nfsmw.h"

#include "include/chloemenulib.h"

auto GetUserProfile() {
	return FEDatabase->mUserProfile;
}

std::string lastState;

void AddCarToCareer(uint32_t handle) {
	auto cars = &GetUserProfile()->PlayersCarStable;
	if (auto out = cars->CreateNewCareerCar(cars, handle)) {
		lastState = "Car added to career";
		return;
	}
	lastState = "Failed to add car to career";
}

bool bCopiedCustomizationsValid = false;
FECustomizationRecord CopiedCustomizations;

void ValueEditorMenu(float& value) {
	ChloeMenuLib::BeginMenu();

	static char inputString[1024] = {};
	ChloeMenuLib::AddTextInputToString(inputString, 1024, true);
	ChloeMenuLib::SetEnterHint("Apply");

	if (DrawMenuOption(inputString + (std::string)"...", "", false, false) && inputString[0]) {
		value = std::stof(inputString);
		memset(inputString,0,sizeof(inputString));
		ChloeMenuLib::BackOut();
	}

	ChloeMenuLib::EndMenu();
}

template<typename T>
void ValueEditorMenu(T& value) {
	ChloeMenuLib::BeginMenu();

	static char inputString[1024] = {};
	ChloeMenuLib::AddTextInputToString(inputString, 1024, true);
	ChloeMenuLib::SetEnterHint("Apply");

	if (DrawMenuOption(inputString + (std::string)"...", "", false, false) && inputString[0]) {
		value = std::stoi(inputString);
		memset(inputString,0,sizeof(inputString));
		ChloeMenuLib::BackOut();
	}

	ChloeMenuLib::EndMenu();
}

template<typename T>
void QuickValueEditor(const char* name, T& value) {
	if (DrawMenuOption(std::format("{} - {}", name, value))) { ValueEditorMenu<T>(value); }
}

void QuickValueEditor(const char* name, float& value) {
	if (DrawMenuOption(std::format("{} - {}", name, value))) { ValueEditorMenu(value); }
}

void PresetCarEditor() {
	ChloeMenuLib::BeginMenu();

	if (TheGameFlowManager.CurrentGameFlowState != 3) {
		DrawMenuOption("Enter the main menu first!");
		ChloeMenuLib::EndMenu();
		return;
	}

	auto cars = &GetUserProfile()->PlayersCarStable;

	if (DrawMenuOption("Cars")) {
		ChloeMenuLib::BeginMenu();

		for (auto& car : cars->CarTable) {
			auto name = FECarRecord::GetDebugName(&car);
			if (!name) continue;
			if (DrawMenuOption(std::format("{} - {}", &car - &cars->CarTable[0], name ? name : "(null)"))) {
				ChloeMenuLib::BeginMenu();

				DrawMenuOption(std::format("Handle - {:X}", car.Handle));
				DrawMenuOption(std::format("FEKey - {:X}", car.FEKey));
				DrawMenuOption(std::format("VehicleKey - {:X}", car.VehicleKey));
				DrawMenuOption(std::format("FilterBits - {:X}", car.FilterBits));
				DrawMenuOption(std::format("Customization - {}", car.Customization));
				DrawMenuOption(std::format("CareerHandle - {}", car.CareerHandle));

				if (car.Handle != 0xFFFFFFFF && DrawMenuOption("Preview")) {
					static RideInfo info;
					RideInfo::Init(&info, -1, CarRenderUsage_Player, false, false);
					FEPlayerCarDB::BuildRideForPlayer(cars, car.Handle, 0, &info);
					GarageMainScreen::SetRideInfo(GarageMainScreen::GetInstance(), &info, 0);
				}

				auto customization = FEPlayerCarDB::GetCustomizationRecordByHandle(cars, car.Customization);
				if (customization && DrawMenuOption("Copy Tuning")) {
					CopiedCustomizations = *customization;
					bCopiedCustomizationsValid = true;
				}

				if (customization && bCopiedCustomizationsValid && DrawMenuOption("Paste Tuning")) {
					*customization = CopiedCustomizations;
				}

				if (auto career = FEPlayerCarDB::GetCareerRecordByHandle(cars, car.CareerHandle); career && DrawMenuOption("Career Stats")) {
					ChloeMenuLib::BeginMenu();

					QuickValueEditor("mMaxBusted", career->TheImpoundData.mMaxBusted);
					QuickValueEditor("mTimesBusted", career->TheImpoundData.mTimesBusted);
					QuickValueEditor("VehicleHeat", career->VehicleHeat);
					QuickValueEditor("Bounty", career->Bounty);
					QuickValueEditor("NumEvadedPursuits", career->NumEvadedPursuits);
					QuickValueEditor("NumBustedPursuits", career->NumBustedPursuits);

					ChloeMenuLib::EndMenu();
				}

				if (car.Handle != 0xFFFFFFFF && DrawMenuOption("Add Car to Career")) {
					AddCarToCareer(car.Handle);
				}

				ChloeMenuLib::EndMenu();
			}
		}

		ChloeMenuLib::EndMenu();
	}

	if (DrawMenuOption("Add Preset Car Manually")) {
		ChloeMenuLib::BeginMenu();
		static char tmp[1024] = "";
		ChloeMenuLib::AddTextInputToString(tmp, 1024, false);
		DrawMenuOption(std::format("Name: {}", tmp));
		if (tmp[0]) {
			if (DrawMenuOption("Add Car")) {
				FEPlayerCarDB::CreateNewPresetCar(cars, tmp);
			}
		}
		ChloeMenuLib::EndMenu();
	}

	if (DrawMenuOption("Markers")) {
		ChloeMenuLib::BeginMenu();

		const char* markerNames[] = {
				"BRAKES",
				"ENGINE",
				"NITROUS",
				"SUPERCHARGER",
				"SUSPENSION",
				"TIRES",
				"TRANSMISSION",
				"BODY",
				"HOOD",
				"SPOILER",
				"RIMS",
				"ROOF_SCOOP",
				"VINYL",
				"DECAL",
				"PAINT",
				"CUSTOM_HUD",
				"GET_OUT_OF_JAIL",
				"PINK_SLIP",
				"CASH",
				"ADD_IMPOUND_BOX",
				"RELEASE_IMPOUND",
		};

		for (int i = FEMarkerManager::MARKER_FIRST; i <= FEMarkerManager::MARKER_IMPOUND_RELEASE; i++) {
			int count = FEMarkerManager::GetNumMarkers(&TheFEMarkerManager, i, 0);
			if (DrawMenuOption(std::format("Add {} Marker ({})", markerNames[i-1], count))) {
				FEMarkerManager::AddMarkerToInventory(&TheFEMarkerManager, i, 0);
			}
			if (count > 0 && DrawMenuOption(std::format("Remove {} Marker ({})", markerNames[i-1], count))) {
				FEMarkerManager::UtilizeMarker(&TheFEMarkerManager, i, 0);
			}
		}

		ChloeMenuLib::EndMenu();
	}

	ChloeMenuLib::EndMenu();
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x3C4040) {
				MessageBoxA(nullptr, "Unsupported game version! Make sure you're using v1.3 (.exe size of 6029312 bytes)", "nya?!~", MB_ICONERROR);
				return TRUE;
			}

			ChloeMenuLib::RegisterMenu("Car Record Editor", &PresetCarEditor);
		} break;
		default:
			break;
	}
	return TRUE;
}