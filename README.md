# distsys-mp2

## Setting up dev environemt for development on Windows OS.
Machine problems MP1 and MP2 need Linux libraries. It will not work on a windows machine. We can use windows feature WSL to develop in Linux environment in Windows OS. 

### Intro to WSL 
https://learn.microsoft.com/en-us/windows/wsl/about 

### Setup dev environment using visual studio code. 

Following links explain everything in detail. 

* Extension - https://code.visualstudio.com/docs/remote/wsl 

* Tutorial - https://code.visualstudio.com/docs/remote/wsl-tutorial 

Following sections are the steps that I performed to bring WSL environment up and running on my Windows machine using various web resources including aforementioned ones. 

### Install Windows Terminal 
This is optional - https://learn.microsoft.com/en-us/windows/terminal/install 

However, this is very powerful specially when working in multiple types of shells. 

### Installation and setup of WSL 
1. Install WSL on the windows machine.  
 * **Installation instructions**: https://learn.microsoft.com/en-us/windows/wsl/install?source=recommendations 

 * Use Ubuntu distribution for installation. 

2. Upgrade from WSL to WSL2  
**wsl --set-version Ubuntu 2**

3. After installing WSL (with Ubuntu distribution) we can open the terminal in Ubuntu environment. 

![Terminal](https://github.com/AshutoshAgarwal01/distsys-mp2/blob/main/Markdown/Images/UbuntuInTerminal.png)

4. WSL mounts a network drive to host Linux environment. 
  * WSL is here: \\wsl$\Ubuntu 
  * We can find it using "explorer.exe ." In Ubuntu console window as well. 
  
5. We should map it as a drive for ease of access. 

![Terminal](https://github.com/AshutoshAgarwal01/distsys-mp2/blob/main/Markdown/Images/AsNetworkDrive.png)

6. Windows folders can be accessed as below from WSL console. 
Use '/mnt/' before windows path. 

![OpenWinInWsl](https://github.com/AshutoshAgarwal01/distsys-mp2/blob/main/Markdown/Images/OpenWindowsFolderInWsl.png)

### Setup development environment using Visual Studio Code. 

1. Install Visual Studio Code in Windows (not in WSL environment). 
2. Install "Remote Development" extension in Visual Studio Code. 
3. Switch between windows and WSL environments in Visual Studio code: 
  
  
  * a\. Switch from Windows to WSL environment
  
    In following snapshot, a folder is open in Windows environment. The terminal will not understand Linux here. 
  
  ![WinToWsl](https://github.com/AshutoshAgarwal01/distsys-mp2/blob/main/Markdown/Images/WindowsToWsl1.png)
  
  Following is the result after performing steps above. 
  
  ![WinToWsl2](https://github.com/AshutoshAgarwal01/distsys-mp2/blob/main/Markdown/Images/WindowsToWsl2.png)
  
  
  * b\. Switch from WSL to Windows environment
  
  ![WslToWin](https://github.com/AshutoshAgarwal01/distsys-mp2/blob/main/Markdown/Images/WSLToWindows1.png)
  
  Result of switching to Windows. 
  
  ![WslToWin2](https://github.com/AshutoshAgarwal01/distsys-mp2/blob/main/Markdown/Images/WslToWindows2.png)
  
### Setup C++ working environment 

1. Install compiler: https://sourceforge.net/projects/mingw/ 
2. Install extensions in Visual studio to make life easier. 
  * **C/C++**: https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools 
  * **C/ C++ extension pack**: https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack 
  
  ![Extn](https://github.com/AshutoshAgarwal01/distsys-mp2/blob/main/Markdown/Images/VsCodeExtensions.png)

