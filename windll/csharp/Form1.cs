#region README

	//_____________________________________________________________________________
	//
	//Sample C# code, .NET Framework 1.1, contributed to the Info-Zip project by
	//Adrian Maull, April 2005.
	//
	//If you have questions or comments, contact me at adrian.maull@sprintpcs.com.  Though
	//I will try to respond to coments/questions, I do not guarantee such response.
	//
	//THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	//KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	//IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
	//PARTICULAR PURPOSE.
	//
	//_____________________________________________________________________________


#endregion

using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;

namespace CSharpInfoZip_UnZipSample
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class Form1 : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Button btnUnzipArchive;
		private System.Windows.Forms.OpenFileDialog openFileDialog1;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.TextBox textBox1;
		private System.Windows.Forms.Label lblProgress;
		private System.Windows.Forms.ProgressBar prgBar;

		//Define the Unzip object
		Unzip m_Unzip = new Unzip();
		private double m_CurrentSize;
		private System.Windows.Forms.Button btnListZipFiles;


		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public Form1()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.btnUnzipArchive = new System.Windows.Forms.Button();
			this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
			this.label1 = new System.Windows.Forms.Label();
			this.textBox1 = new System.Windows.Forms.TextBox();
			this.lblProgress = new System.Windows.Forms.Label();
			this.prgBar = new System.Windows.Forms.ProgressBar();
			this.btnListZipFiles = new System.Windows.Forms.Button();
			this.SuspendLayout();
			//
			// btnUnzipArchive
			//
			this.btnUnzipArchive.Location = new System.Drawing.Point(8, 24);
			this.btnUnzipArchive.Name = "btnUnzipArchive";
			this.btnUnzipArchive.Size = new System.Drawing.Size(96, 24);
			this.btnUnzipArchive.TabIndex = 0;
			this.btnUnzipArchive.Text = "Unzip archive...";
			this.btnUnzipArchive.Click += new System.EventHandler(this.btnUnzipArchive_Click);
			//
			// label1
			//
			this.label1.Location = new System.Drawing.Point(8, 64);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(184, 16);
			this.label1.TabIndex = 1;
			this.label1.Text = "Unzip DLL Print callback message:";
			//
			// textBox1
			//
			this.textBox1.Location = new System.Drawing.Point(8, 80);
			this.textBox1.Multiline = true;
			this.textBox1.Name = "textBox1";
			this.textBox1.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
			this.textBox1.Size = new System.Drawing.Size(464, 120);
			this.textBox1.TabIndex = 2;
			this.textBox1.Text = "";
			//
			// lblProgress
			//
			this.lblProgress.Location = new System.Drawing.Point(8, 208);
			this.lblProgress.Name = "lblProgress";
			this.lblProgress.Size = new System.Drawing.Size(216, 16);
			this.lblProgress.TabIndex = 3;
			//
			// prgBar
			//
			this.prgBar.Location = new System.Drawing.Point(8, 224);
			this.prgBar.Name = "prgBar";
			this.prgBar.Size = new System.Drawing.Size(216, 16);
			this.prgBar.TabIndex = 4;
			//
			// btnListZipFiles
			//
			this.btnListZipFiles.Location = new System.Drawing.Point(120, 24);
			this.btnListZipFiles.Name = "btnListZipFiles";
			this.btnListZipFiles.Size = new System.Drawing.Size(96, 24);
			this.btnListZipFiles.TabIndex = 5;
			this.btnListZipFiles.Text = "List zip files...";
			this.btnListZipFiles.Click += new System.EventHandler(this.btnListZipFiles_Click);
			//
			// Form1
			//
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(480, 254);
			this.Controls.Add(this.btnListZipFiles);
			this.Controls.Add(this.prgBar);
			this.Controls.Add(this.lblProgress);
			this.Controls.Add(this.textBox1);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.btnUnzipArchive);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "Form1";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
			this.Text = "Form1";
			this.ResumeLayout(false);

		}
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main()
		{
			Application.Run(new Form1());
		}

		#region Event Handlers

		private void btnUnzipArchive_Click(object sender, System.EventArgs e)
		{
			openFileDialog1.ShowDialog();
			string file = openFileDialog1.FileName;

			if (file == null | file == string.Empty) return;

			//Instantiate the Unzip object
			m_Unzip = new Unzip();

			//NOTE:
			//There are many unzip options.  This sample just demonstrates basic unzip options.
			//Consult the InfoZip documentation for more option information.

			//Set the Unzip object properties
			m_Unzip.ZipFileName = file;
			m_Unzip.HonorDirectories = HonorDirectoriesEnum.True;
			m_Unzip.ExtractList = ExtractListEnum.Extract;

			//PROBLEM:
			//I receive an error from the DLL when this flag is true.  When this is true, the DLL extracts the
			//zip file comment, which throws an error.  Look at the KNOWN ISSUES region in the Unzip.cs for
			//futher comments
			//m_Unzip.Verbose = VerboseEnum.True;

			//NOTE:
			//Directory where the unzipped files are stored.  Change this as appropriate
			m_Unzip.ExtractDirectory = @"c:\tmp\unzip";

			//Wire the event handlers to receive the events from the Unzip class
			m_Unzip.ReceivePrintMessage +=new UnZipDLLPrintMessageEventHandler(unZip_ReceivePrintMessage);
			m_Unzip.ReceiveServiceMessage +=new UnZipDLLServiceMessageEventHandler(unZip_ReceiveServiceMessage);

			//Unzip the files
			int ret = m_Unzip.UnZipFiles();

			//Examine the return code
			MessageBox.Show("Done.  Return Code: " + ret.ToString());
		}

		private void btnListZipFiles_Click(object sender, System.EventArgs e)
		{

			openFileDialog1.ShowDialog();
			string file = openFileDialog1.FileName;

			if (file == null | file == string.Empty) return;

			//Instantiate the Unzip object
			m_Unzip = new Unzip();

			//NOTE:
			//There are many unzip options.  This sample just demonstrates basic unzip options.
			//Consult the InfoZip documentation for more option information.

			//Set the Unzip object properties
			m_Unzip.ZipFileName = file;
			m_Unzip.HonorDirectories = HonorDirectoriesEnum.True;
			m_Unzip.ExtractList = ExtractListEnum.ListContents;

			//NOTE:
			//Directory where the unzipped files are stored.  Change this as appropriate
			m_Unzip.ExtractDirectory = @"c:\tmp\unzip";

			//PROBLEM:
			//Whenever I try to retrieve the comment I get an error from the DLL.  If you want to try it, just set the
			// m_Unzip.DisplayComment = DisplayCommentEnum.True

			//Wire the event handlers to receive the events from the Unzip class
			m_Unzip.ReceivePrintMessage +=new UnZipDLLPrintMessageEventHandler(unZip_ReceivePrintMessage);
			m_Unzip.ReceiveServiceMessage +=new UnZipDLLServiceMessageEventHandler(unZip_ReceiveServiceMessage);

			//Unzip the files
			ZipFileEntries zfes = m_Unzip.GetZipFileContents();

			//Show the file contents
			frmShowContents frm = new frmShowContents();
			frm.UnzippedFileCollection = zfes;

			//WORK AROUND:
			frm.Comment = m_Unzip.GetZipFileComment();

			frm.ShowDialog(this);

			//Examine the return code
			MessageBox.Show("Done.");

		}

		private void unZip_ReceivePrintMessage(object sender, UnZipDLLPrintMessageEventArgs e)
		{
			textBox1.Text = e.PrintMessage + "\r\n";
			Application.DoEvents();
		}

		private void unZip_ReceiveServiceMessage(object sender, UnZipDLLServiceMessageEventArgs e)
		{
			m_CurrentSize += e.SizeOfFileEntry;
			prgBar.Value = (Convert.ToInt32(100 * (m_CurrentSize/e.ZipFileSize)));
			lblProgress.Text = "Unzipping " + m_CurrentSize.ToString() + " of " + e.ZipFileSize + " bytes.";
			Application.DoEvents();
		}

		#endregion



	}
}
