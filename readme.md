<h1 align="center" id="heading">Saturn Ring Library</h1>
<p align="center">
 Easy to use SGL wrapper written in C++</br>
 <a href="https://srl.reye.me/"><b>Documentation</b></a></br></br>
 <img src="https://github.com/ReyeMe/SaturnRingLib/blob/main/Documentation/resources/srl_logo.png"></br></br>
 <a href="https://github.com/ReyeMe/SaturnRingLib/issues">
    <img src="https://img.shields.io/github/issues/ReyeMe/SaturnRingLib.svg" alt="Issues"/>
  </a>
  <a href="https://github.com/ReyeMe/SaturnRingLib/commits/main">
    <img src="https://img.shields.io/github/last-commit/ReyeMe/SaturnRingLib/main.svg" alt="Last commit"/>
  </a>
</p>

<div align="center">
  <table>
    <tr>
      <th colspan=4 style="text-align:center;">Brought to you by</th>
    </tr>
    <tr>
      <td><a href="https://github.com/ReyeMe" target="_blank"><img src="https://github.com/ReyeMe.png" width="32px;"/></a></td>
      <td><a href="https://github.com/ReyeMe" target="_blank">ReyeMe</a></td>
      <td><a href="https://github.com/robertoduarte" target="_blank"><img src="https://github.com/robertoduarte.png" width="32px;"/></a></td>
      <td><a href="https://github.com/robertoduarte" target="_blank">robertoduarte</a></td>
    </tr>
    <tr>
      <td><a href="https://github.com/seven-shades" target="_blank"><img src="https://github.com/seven-shades.png" width="32px;"/></a></td>
      <td><a href="https://github.com/seven-shades" target="_blank">7shades</a></td>
      <td><a href="https://github.com/willll" target="_blank"><img src="https://github.com/willll.png" width="32px;"/></a></td>
      <td><a href="https://github.com/willll" target="_blank">willll</a></td>
    </tr>
    <tr>
      <td><a href="https://github.com/nemesis-saturn" target="_blank"><img src="https://github.com/nemesis-saturn.png" width="32px;"/></a></td>
      <td><a href="https://github.com/nemesis-saturn" target="_blank">nemesis-saturn</a></td>
      <td><a href="https://github.com/jae686" target="_blank"><img src="https://github.com/jae686.png" width="32px;"/></a></td>
      <td><a href="https://github.com/jae686" target="_blank">jae686</a></td>
    </tr>
  </table>
</div>

## Getting started
### Installing SRL
#### Download
<details>
    <summary>Using pre-packaged release</summary>

Go to the [Releases](https://github.com/ReyeMe/SaturnRingLib/releases) section and download latest .zip release.

</details>
<details>
    <summary>Using git repository</summary>
Clone git repository by using:

```
git clone --recurse-submodules https://github.com/ReyeMe/SaturnRingLib.git
```

> __Note:__ It is important to not forget the ``--recurse-submodules`` otherwise some submodules (SaturnMath++ and TLSF memory allocator) will not get downloaded.
</details>

#### Setting up
1. Download toolchain. This can be done by running ``setup_compiler.bat``.  
2. Download <a href="https://mednafen.github.io/" target="_blank">mednafen</a> and put it into ``emulators/mednafen/`` folder (or install it with a package manager if on linux).  
You will also need to obtain bios file (``mpr-17933.bin``) and put it in the mednafen ``firmware`` folder. 

> __Note:__ ``.bat`` scripts used within SRL can be run natively on Windows, Linux or Mac.
<details>
  <summary>Linux dependencies</summary>

Use your preferred package manager to install the following:
- `make` - for compilation
- `unzip` - used during compiler installation
- `wget` - to download the compiler
- `sox` - to convert audio
    - `libsox-fmt-mp3` - for sox to support mp3 files
- `xorriso` - to build cue/bin

</details>

#### Build and run samples
Samples and project in SRL can be build and run from VSCode or manually by starting ``.bat`` scripts in the sample/project directory
<details>
    <summary>With VSCode (recommended)</summary>

1. Open the folder of a project/sample (folder contains .vscode sub folder) with VSCode.  
2. Open tasks menu using ``CTRL+SHIFT+B``.
3. Click on one of the ``compile`` tasks to build the project, or ``run with`` task to start emulator.  
Projects can be compiled with DEBUG or RELEASE target.

</details>
<details>
    <summary>Manually</summary>

1. Open the folder of a project/sample.
2. To build just run ``compile.bat`` or to run a built project in an emulator use one of the ``run with`` ``.bat`` files.

</details>

#### Creating a project
To create a new custom project just copy one of the samples to the projects folder.  
The name of the project can than be changed within the ``makefile``.

<a href="https://github.com/jae686/srl-tutorials/blob/main/README.md" target="_blank">
  <img src="https://img.shields.io/badge/Check out tutorial series by jae686 for more.-blue" alt="Check out tutorial series by jae686 for more."/>
</a>

# Contributing 
<a href="https://github.com/ReyeMe/SaturnRingLib/blob/main/.github/CONTRIBUTING.md">
  <img src="https://img.shields.io/badge/PRs-welcome-blue" alt="PRs Welcome"/>
</a>

Contributions are welcome! Please read our [Contributing Guidelines](https://github.com/ReyeMe/SaturnRingLib/blob/main/.github/CONTRIBUTING.md) before submitting an issue or a pull request.