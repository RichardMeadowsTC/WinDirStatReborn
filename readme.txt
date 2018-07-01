The project builds using
  Microsoft Visual Studio Community 2017 
  Version 15.7.4
  VisualStudio.15.Release/15.7.4+27703.2035
  Microsoft .NET Framework
  Version 4.7.03056

You will need to install .Net 4.7.2 SDK.  Before opening the solution.  This version of .Net ships with Windows 10 April 2018 Update 
version 1803 (Build 17134.x).  A copy of the SDK and redistributable can be found in Net-4.7.2 folder.

Project was inspired by WinDirStat. Unfortunatly, I have not been able to find any copy of WinDirStat that builds master branch.
I have also tried getting zip down load from Source Forge.  The zip is missing ..\common\platform.cpp file and has other issues.

I would like to have a WinDirStat that can export directory size report. The binary I use from Source Forge has option
to send a report in email, but it crashes. If I am lucky, I can get a copy of the report to my clipboard before it crashes.
A better solution is needed.

Don't intend to implement all the features of WinDirStat.  Just enough to get directory usage. Folder sizes and possibly percentage information.
There will be a way to export or copy past information out of the application.  As a flat report. Possibly as dynamic html that can be 
interacted with out side of the app.