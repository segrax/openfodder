/*
 *  Open Fodder
 *  ---------------
 *
 *  Copyright (C) 2008-2018 Open Fodder
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "stdafx.hpp"
#include <chrono>

cUnitTesting::cUnitTesting() {

    g_Fodder->mStartParams.mSinglePhase = true;
    g_Fodder->mStartParams.mSkipBriefing = true;
    g_Fodder->mStartParams.mSkipIntro = true;
    g_Fodder->mStartParams.mSkipService = true;
    g_Fodder->mStartParams.mSkipRecruit = true;

    if (!g_Fodder->mStartParams.mDemoRecord)
        g_Fodder->mStartParams.mDemoPlayback = true;
}

std::string cUnitTesting::getCurrentTestFileName() {
    std::string MissionPhase = "";
    MissionPhase += "m" + std::to_string(g_Fodder->mGame_Data.mMission_Number);
    MissionPhase += "p" + std::to_string(g_Fodder->mGame_Data.mMission_Phase);
    MissionPhase += ".ofd";
    return MissionPhase;
}

std::string cUnitTesting::getCurrentTestName() {
    std::string MissionTitle = "Mission " + std::to_string(g_Fodder->mGame_Data.mMission_Number);
    MissionTitle += " Phase " + std::to_string(g_Fodder->mGame_Data.mMission_Phase);
    MissionTitle += ": " + g_Fodder->mGame_Data.mMission_Current->mName;
    if(g_Fodder->mGame_Data.mPhase_Current->mName != g_Fodder->mGame_Data.mMission_Current->mName)
        MissionTitle += " (" + g_Fodder->mGame_Data.mPhase_Current->mName + ")";
    return MissionTitle;

}
void cUnitTesting::EngineSetup() {
    g_Fodder->mIntroDone = false;

    g_Fodder->mSurface->palette_SetToBlack();
    g_Fodder->mSurface->paletteNew_SetToBlack();
    g_Fodder->mSurface->surfaceSetToPalette();
    g_Fodder->mSurface->resetPaletteAdjusting();

    g_Fodder->mPhase_TryingAgain = false;
    g_Fodder->Mouse_Setup();
}

bool cUnitTesting::RunTests(const std::string pCampaign) {
    bool Retry = false;
    g_Fodder->Game_Setup();

    if (g_Fodder->mParams.mUnitTesting && g_Fodder->mParams.mDemoPlayback)
        g_Fodder->mParams.mSleepDelta = 0;


    while (g_Fodder->mGame_Data.mMission_Current) {
        EngineSetup();

        // Set demo file name
        g_Fodder->mParams.mDemoFile = local_PathGenerate(getCurrentTestFileName(), g_Fodder->mParams.mCampaignName, eTest);

        std::string MissionTitle = getCurrentTestName();

        if (g_Fodder->mStartParams.mDemoRecord && !Retry) {
            if (local_FileExists(g_Fodder->mParams.mDemoFile)) {
                g_Debugger->Notice("Test exists for " + MissionTitle + ", skipping");
                g_Fodder->mGame_Data.Phase_Next();
                continue;
            }
        }

        Retry = false;
        g_Debugger->TestStart(MissionTitle, g_Fodder->mParams.mCampaignName);

        if (g_Fodder->mStartParams.mDemoPlayback) {
            if (!g_Fodder->mStartParams.mDemoRecord) {
                if (!g_Fodder->Demo_Load()) {
                    g_Debugger->TestComplete(MissionTitle, g_Fodder->mParams.mCampaignName, "No test found", 0, eTest_Skipped);
                    g_Fodder->mGame_Data.Phase_Next();
                    continue;
                }
            }

            g_Fodder->mGame_Data.mDemoRecorded.playback();
        }

        if (g_Fodder->mStartParams.mDemoRecord) {
            if (g_Fodder->mStartParams.mDemoRecordResumeCycle) {
                g_Debugger->Notice("Resuming " + MissionTitle);;
            }
            else {
                g_Fodder->mGame_Data.mDemoRecorded.clear();
                g_Debugger->Notice("Recording " + MissionTitle);
            }
        }

        // Reset demo status
        g_Fodder->mParams.mDemoRecord = g_Fodder->mStartParams.mDemoRecord;
        g_Fodder->mParams.mDemoPlayback = g_Fodder->mStartParams.mDemoPlayback;
        g_Fodder->mParams.mAppVeyor = g_Fodder->mStartParams.mAppVeyor;
        // Keep game state
        g_Fodder->mGame_Data_Backup = g_Fodder->mGame_Data;

        // Run the phase
        auto missionStartTime = std::chrono::steady_clock::now();
        auto res = g_Fodder->Mission_Loop();
        auto missionDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - missionStartTime);

        // If recording
        if (g_Fodder->mStartParams.mDemoRecord) {

            if (g_Fodder->mPhase_Complete)
                g_Fodder->mGame_Data.mDemoRecorded.save();

            else {

                Retry = true;
                // If the phase was aborted (ESC key), don't replay it.. start over
                if (!g_Fodder->mPhase_Aborted) {

                    // Less than 10 cycles, player can start over
                    if (g_Fodder->mGame_Data.mGameTicks - 10 > 0) {
                        g_Fodder->mStartParams.mDemoRecordResumeCycle = (g_Fodder->mGame_Data.mGameTicks - 10);
                        g_Fodder->mStartParams.mDemoPlayback = true;

                        auto Demo = g_Fodder->mGame_Data.mDemoRecorded;
                        Demo.removeFrom(g_Fodder->mStartParams.mDemoRecordResumeCycle);
                        g_Fodder->mGame_Data_Backup.mDemoRecorded = Demo;
                        g_Fodder->mStartParams.mSleepDelta = 0;
                    }
                }

                g_Fodder->mGame_Data = g_Fodder->mGame_Data_Backup;
                continue;
            }
        }

        if (!g_Fodder->mPhase_Complete) {
            g_Debugger->TestComplete(MissionTitle, g_Fodder->mParams.mCampaignName, "Phase Failed", (size_t)missionDuration.count(), eTest_Failed);
            return false;
        }

        g_Debugger->TestComplete(MissionTitle, g_Fodder->mParams.mCampaignName, "Phase Complete", (size_t)missionDuration.count(), eTest_Passed);

        g_Fodder->mGame_Data = g_Fodder->mGame_Data_Backup;
        g_Fodder->mGame_Data.Phase_Next();
    }

    return true;
}

bool cUnitTesting::Start() {
    std::vector<std::string> Campaigns;

    // No Campaign Name
    if (!g_Fodder->mStartParams.mCampaignName.size())
        Campaigns = g_Fodder->mVersions->GetCampaignNames();
    else
        Campaigns.push_back(g_Fodder->mParams.mCampaignName);

    bool Result = true;
    for (auto& CampaignName : Campaigns) {
        g_Fodder->mParams = g_Fodder->mStartParams;
        g_Fodder->mParams.mCampaignName = CampaignName;

        g_Debugger->TestStart(CampaignName, "Campaign");

        g_Fodder->mGame_Data.mCampaign.Clear();
        if (!g_Fodder->Campaign_Load(CampaignName)) {
            g_Debugger->Error("Campaign " + CampaignName + " not found");
            g_Debugger->TestComplete(CampaignName, "Campaign", "not found", 0, eTest_Skipped);
            continue;
        }

        // FIXME: Create test folder
        if (!local_FileExists(local_PathGenerate("", CampaignName, eTest)) && g_Fodder->mStartParams.mDemoRecord) {

            std::string Command = "mkdir \"" + local_PathGenerate("", CampaignName, eTest) + "\"";
            system(Command.c_str());
        }

        auto campaignStartTime = std::chrono::steady_clock::now();
        if (!RunTests(CampaignName))
            Result = false;
        auto campaignDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - campaignStartTime);

        g_Debugger->TestComplete(CampaignName, "Campaign", "", (size_t)campaignDuration.count(), Result ? eTest_Passed : eTest_Failed);
    }
    return Result;
}
