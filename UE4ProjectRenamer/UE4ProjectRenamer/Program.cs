/*
 * 
 *  This is just a really quick mockup of a program to rename base path UE4 projects, nothing fancy and nothing all that well executed, I am very c# rusty. 
 * 
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
namespace UE4ProjectRenamer
{
    class Program
    {
        static void Main(string[] args)
        {
            string FilePath = Path.GetDirectoryName(new Uri(System.Reflection.Assembly.GetEntryAssembly().CodeBase).LocalPath);
            string [] FoundFiles = Directory.GetFiles(FilePath, "*.uproject", SearchOption.TopDirectoryOnly);

            if (FoundFiles.Length < 1)
            {
                MessageBox.Show("Uproject not found in .exe directory, please move this .exe to the uproject directory prior to running");
                return;
            }

            string TargetProject = FoundFiles[0];

            if(MessageBox.Show("Found uproject by name of: " + Path.GetFileName(TargetProject) + " - Do you want to rename this project?", "Rename this project?", MessageBoxButtons.OKCancel) == DialogResult.Cancel)
            {
                return;
            }

            string OldProjectName = Path.GetFileNameWithoutExtension(TargetProject);
            string NewProjectName = Microsoft.VisualBasic.Interaction.InputBox("Enter the new project name for: " + OldProjectName, "EnterNewName", "Default", -1, -1);

            if (NewProjectName == "Default" || NewProjectName.Length < 1)
                return;

            if (MessageBox.Show("Preparing to rename: " + OldProjectName + " to " + NewProjectName + ", Are you sure?", "Rename this project?", MessageBoxButtons.OKCancel) == DialogResult.Cancel)
            {
                return;
            }

            // Remove old solution files
            File.Delete(FilePath + "\\" + OldProjectName + ".sln");
            File.Delete(FilePath + "\\" + OldProjectName + ".sdf");
            File.Delete(FilePath + "\\" + OldProjectName + ".VC.db");

            // Replace the .uproject
            OpenAndRenameFile(TargetProject, OldProjectName, NewProjectName);


            List<string> FileList = new List<string>();
            GetFilesInDirectory(FilePath + "\\Source", ref FileList);
        
            // Replace all interior source and .cs files
            foreach(string file in FileList)
            {
                OpenAndRenameFile(file, OldProjectName, NewProjectName);
            }

            // Rename source folder
            Directory.Move(FilePath + "\\Source\\" + OldProjectName, FilePath + "\\Source\\" + NewProjectName);

            MessageBox.Show("Finished renaming all files in source directory, you will need to re-generate your project files now, rename the project folder, and change the name in defaultengine.ini");
        }

        static bool OpenAndRenameFile(string FileName, string OldProjectName, string NewProjectName)
        {
            try
            {
                string text = File.ReadAllText(FileName);
                text = text.Replace(OldProjectName.ToUpper() + "_API", NewProjectName.ToUpper() + "_API");
                text = text.Replace(OldProjectName, NewProjectName);

                string newFileName = Path.GetDirectoryName(FileName) + "\\" + Path.GetFileName(FileName).Replace(OldProjectName, NewProjectName);
                File.WriteAllText(newFileName, text);

                if (FileName != newFileName && File.Exists(FileName))
                {
                    File.Delete(FileName);
                }
            }
            catch(Exception exp)
            {
                MessageBox.Show("Failed to open and modify " + Path.GetFileName(FileName) + "! " + exp.Message);
                return false;
            }

            return true;
        }

        static bool GetFilesInDirectory(string nDirectory, ref List<string> StringList)
        {
            string[] FoundFiles = Directory.GetFiles(nDirectory, "*", SearchOption.AllDirectories);

            StringList.AddRange(FoundFiles);
            return true;
        }
    }
}
