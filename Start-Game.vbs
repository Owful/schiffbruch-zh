Option Explicit
Dim shell, files, folder
Set shell = CreateObject("WScript.Shell")
Set files = CreateObject("Scripting.FileSystemObject")
folder = files.BuildPath(files.GetParentFolderName(WScript.ScriptFullName), "bin")
shell.CurrentDirectory = folder
shell.Run Chr(34) & files.BuildPath(folder, "Schiffbruch.exe") & Chr(34), 1, False
