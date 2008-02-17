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

#region KNOWN ISSUES

	//_____________________________________________________________________________
	//
	//KNOWN ISSUES:
	//From my testing I have encountered some issues
	//
	//1.	I receive an error code from the Unzip32.dll when I try to retrieve the comment from the
	//		zip file.  To display a comment you set the nzflag of the DCLIST structure = 1.  In this
	//		implementation, just set the m_Unzip.ExtractList = ExtractListEnum.ListContents and
	//		m_Unzip.DisplayComment = DisplayCommentEnum.True
	//		I provided a work around to this in the GetZipFileComment() method.
	//
	//2.	I receive an error code from the Unzip32.dll when I try to set m_Unzip.Verbose = VerboseEnum.True,
	//		which is the equivalent of nZIflag = 1.
	//
	//3.	I have not tested any password/encryption logic in this sample
	//_____________________________________________________________________________


#endregion

using System;
using System.Security.Permissions;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace CSharpInfoZip_UnZipSample
{
	/// <summary>
	/// Summary description for Unzip.
	/// </summary>

	#region Public Enums

	public enum ExtractOnlyNewerEnum {False, True};
	public enum SpaceToUnderScoreEnum {False, True};
	public enum PromptToOverWriteEnum {NotRequired, Required};
	public enum QuietEnum {AllMessages, LessMessages, NoMessages};
	public enum WriteStdOutEnum {False, True};
	public enum TestZipEnum {False, True};
	public enum ExtractListEnum {Extract, ListContents};
	public enum FreshenExistingEnum {False, True};
	public enum DisplayCommentEnum {False, True};
	public enum HonorDirectoriesEnum {False, True};
	public enum OverWriteFilesEnum {False, True};
	public enum ConvertCR_CRLFEnum {False, True};
	public enum VerboseEnum {False, True};
	public enum CaseSensitivityEnum {True, False};
	public enum PrivilegeEnum {Default, ACL, Privilege};
	public enum ReplaceFileOptionsEnum {OverwritePrompt=100,Overwrite=102,OverwriteAll=103,OverwriteNone=104};


	#endregion

	#region Event Delegates

	public delegate void UnZipDLLPrintMessageEventHandler(object sender, UnZipDLLPrintMessageEventArgs  e);
	public delegate void UnZipDLLServiceMessageEventHandler(object sender, UnZipDLLServiceMessageEventArgs  e);

	#endregion

	public class Unzip
	{
		public Unzip()
		{
		}

		#region Private Vars

		private int m_ZipFileSize;
		private int m_ZipFileCount;
		private int m_Stop;
		private string m_ZipFileName;
		private string m_ExtractionDirectory;
		private string m_Password;
		private string m_Comment;
		private string [] m_FilesToUnzip = new string[0]; //Default
		private string [] m_FilesToExclude = new string[0]; //Default
		private ZipFileEntries m_ZipFileEntries = new ZipFileEntries();
		private ReplaceFileOptionsEnum m_ReplaceFileOption = ReplaceFileOptionsEnum.OverwritePrompt; //Default
		private ASCIIEncoding m_Ascii = new ASCIIEncoding();

		private ExtractOnlyNewerEnum m_ExtractOnlyNewer;
		private SpaceToUnderScoreEnum m_SpaceToUnderScore;
		private PromptToOverWriteEnum m_PromptToOverWrite;
		private QuietEnum m_Quiet;
		private WriteStdOutEnum m_WriteStdOut;
		private TestZipEnum m_TestZip;
		private ExtractListEnum m_ExtractList;
		private FreshenExistingEnum m_FreshenExisting;
		private DisplayCommentEnum m_DisplayComment;
		private HonorDirectoriesEnum m_HonorDirectories;
		private OverWriteFilesEnum m_OverWriteFiles;
		private ConvertCR_CRLFEnum m_ConvertCR_CRLF;
		private VerboseEnum m_Verbose;
		private CaseSensitivityEnum m_CaseSensitivity;
		private PrivilegeEnum m_Privilege;


		#endregion

		#region Structures

		// Callback Large String
		protected struct UNZIPCBChar
		{
			[MarshalAs( UnmanagedType.ByValArray, SizeConst= 32800, ArraySubType = UnmanagedType.U1)]
			public byte [] ch;
		}

		// Callback Small String
		protected struct UNZIPCBCh
		{
			[MarshalAs( UnmanagedType.ByValArray, SizeConst= 256, ArraySubType = UnmanagedType.U1)]
			public byte [] ch;
		}

		//UNZIP32.DLL DCLIST Structure
		[ StructLayout( LayoutKind.Sequential )]
		protected struct DCLIST
		{
			public int ExtractOnlyNewer;		//1 = Extract Only Newer/New, Else 0
			public int SpaceToUnderscore;		//1 = Convert Space To Underscore, Else 0
			public int PromptToOverwrite;		//1 = Prompt To Overwrite Required, Else 0
			public int fQuiet;							//2 = No Messages, 1 = Less, 0 = All
			public int ncflag;							//1 = Write To Stdout, Else 0
			public int ntflag;							//1 = Test Zip File, Else 0
			public int nvflag;							//0 = Extract, 1 = List Zip Contents
			public int nfflag;							//1 = Extract Only Newer Over Existing, Else 0
			public int nzflag;							//1 = Display Zip File Comment, Else 0
			public int ndflag;							//1 = Honor Directories, Else 0
			public int noflag;							//1 = Overwrite Files, Else 0
			public int naflag;							//1 = Convert CR To CRLF, Else 0
			public int nZIflag;						//1 = Zip Info Verbose, Else 0
			public int C_flag;						//1 = Case Insensitivity, 0 = Case Sensitivity
			public int fPrivilege;					//1 = ACL, 2 = Privileges
			public string Zip;						//The Zip Filename To Extract Files
			public string ExtractDir;				//The Extraction Directory, NULL If Extracting To Current Dir
		}

		//UNZIP32.DLL Userfunctions Structure
		[ StructLayout( LayoutKind.Sequential )]
		protected struct USERFUNCTION
		{
			public UZDLLPrintCallback UZDLLPrnt;								//Print function callback
			public int UZDLLSND;														//Not supported
			public UZDLLReplaceCallback UZDLLREPLACE;					//Replace function callback
			public UZDLLPasswordCallback UZDLLPASSWORD;			//Password function callback
			public UZReceiveDLLMessageCallback UZDLLMESSAGE;		//Receive message callback
			public UZDLLServiceCallback UZDLLSERVICE;					//Service callback
			public int TotalSizeComp;												//Total Size Of Zip Archive
			public int TotalSize;														//Total Size Of All Files In Archive
			public int CompFactor;													//Compression Factor
			public int NumMembers;													//Total Number Of All Files In The Archive
			public short cchComment;												//Flag If Archive Has A Comment!
		}

		#endregion

		#region Constants
		#endregion

		#region DLL Function Declares

		//NOTE:
		//This Assumes UNZIP32.DLL Is In Your \Windows\System Directory!
		[DllImport("unzip32.dll", SetLastError=true)]
		private static extern int Wiz_SingleEntryUnzip (int ifnc, string [] ifnv, int xfnc, string [] xfnv,
			ref DCLIST dcl, ref USERFUNCTION Userf);


		#endregion

		#region Properties

		public string Password
		{
			get {return m_Password;}
			set {m_Password = value;}
		}

		public string Comment
		{
			get {return m_Comment;}
			set {m_Comment = value;}
		}

		public ExtractOnlyNewerEnum ExtractOnlyNewer
		{
			get {return m_ExtractOnlyNewer;}
			set {m_ExtractOnlyNewer = value;}
		}

		public SpaceToUnderScoreEnum SpaceToUnderScore
		{
			get {return m_SpaceToUnderScore;}
			set {m_SpaceToUnderScore = value;}
		}

		public PromptToOverWriteEnum PromptToOverWrite
		{
			get {return m_PromptToOverWrite;}
			set {m_PromptToOverWrite = value;}
		}

		public QuietEnum Quiet
		{
			get {return m_Quiet;}
			set {m_Quiet = value;}
		}

		public WriteStdOutEnum WriteStdOut
		{
			get {return m_WriteStdOut;}
			set {m_WriteStdOut = value;}
		}

		public TestZipEnum TestZip
		{
			get {return m_TestZip;}
			set {m_TestZip = value;}
		}

		public ExtractListEnum ExtractList
		{
			get {return m_ExtractList;}
			set {m_ExtractList = value;}
		}

		public FreshenExistingEnum FreshenExisting
		{
			get {return m_FreshenExisting;}
			set {m_FreshenExisting = value;}
		}

		public DisplayCommentEnum DisplayComment
		{
			get {return m_DisplayComment;}
			set {m_DisplayComment = value;}
		}

		public HonorDirectoriesEnum HonorDirectories
		{
			get {return m_HonorDirectories;}
			set {m_HonorDirectories = value;}
		}

		public OverWriteFilesEnum OverWriteFiles
		{
			get {return m_OverWriteFiles;}
			set {m_OverWriteFiles = value;}
		}

		public ConvertCR_CRLFEnum ConvertCR_CRLF
		{
			get {return m_ConvertCR_CRLF;}
			set {m_ConvertCR_CRLF = value;}
		}

		public VerboseEnum Verbose
		{
			get {return m_Verbose;}
			set {m_Verbose = value;}
		}

		public CaseSensitivityEnum CaseSensitivity
		{
			get {return m_CaseSensitivity;}
			set {m_CaseSensitivity = value;}
		}

		public PrivilegeEnum Privilege
		{
			get {return m_Privilege;}
			set {m_Privilege = value;}
		}

		public string ZipFileName
		{
			get {return m_ZipFileName;}
			set {m_ZipFileName = value;}
		}

		public string ExtractDirectory
		{
			get {return m_ExtractionDirectory;}
			set {m_ExtractionDirectory = value;}
		}

		public string [] FilesToUnzip
		{
			get {return m_FilesToUnzip;}
			set {m_FilesToUnzip = value;}
		}

		public string [] FilesToExclude
		{
			get {return m_FilesToExclude;}
			set {m_FilesToExclude = value;}
		}

		#endregion

		#region UnZip DLL Delegates

		//Callback For UNZIP32.DLL - Receive Message Function
		protected delegate void UZReceiveDLLMessageCallback (uint ucsize, uint csiz, ushort cfactor, ushort mo,
																ushort dy, ushort yr, ushort hh, ushort mm, sbyte c, ref UNZIPCBCh fname,
																ref UNZIPCBCh meth, uint crc, sbyte fCrypt);

		//Callback For UNZIP32.DLL - Print Message Function
		protected delegate int UZDLLPrintCallback (ref UNZIPCBChar fname, uint x);

		//Callback For UNZIP32.DLL - DLL Service Function
		protected delegate int UZDLLServiceCallback (ref UNZIPCBChar fname, uint x);

		//Callback For UNZIP32.DLL - Password Function
		protected delegate short UZDLLPasswordCallback (ref UNZIPCBCh p, int n, ref UNZIPCBCh m, ref UNZIPCBCh name);

		//Callback For UNZIP32.DLL - Replace Function To Overwrite Files
		protected delegate int UZDLLReplaceCallback(ref UNZIPCBChar fname);

		#endregion

		#region Events

		public event UnZipDLLPrintMessageEventHandler ReceivePrintMessage;
		public event UnZipDLLServiceMessageEventHandler ReceiveServiceMessage;

		#endregion

		#region Protected Functions

		protected virtual void OnReceivePrintMessage (UnZipDLLPrintMessageEventArgs e)
		{
			if (ReceivePrintMessage != null)
			{
				ReceivePrintMessage(this, e);
			}
		}

		protected virtual void OnReceiveServiceMessage (UnZipDLLServiceMessageEventArgs e)
		{
			if (ReceiveServiceMessage != null)
			{
				ReceiveServiceMessage(this, e);
			}
		}

		#endregion

		#region CallBack Functions

		//This function is called when the DCLIST structure's nvflag is ExtractListEnum.ListContents.
		//It is used to list the zip file contents
		protected void UZReceiveDLLMessage (uint ucsize, uint csiz, ushort cfactor, ushort mo,
															ushort dy, ushort yr, ushort hh, ushort mm, sbyte c, ref UNZIPCBCh fname,
															ref UNZIPCBCh meth, uint crc, sbyte fCrypt)
		{
			string s = string.Empty;
			int i = 0;

			for (i = 0; i <= fname.ch.Length; i ++)
				if (fname.ch[i] == 0) break;
			s = m_Ascii.GetString(fname.ch,0,i);

			//Add up the size of each file in the zip file
			m_ZipFileSize += (int)ucsize;
			m_ZipFileCount ++;

			//NOTE:
			//Build out the ZipFileEntry collection
			//You can do additional formatting for the month, day, and year properties
			ZipFileEntry zfe = new ZipFileEntry();
			zfe.FileName = Path.GetFileName(s);
			zfe.FilePath = Path.GetDirectoryName(s);
			zfe.IsFolder = (zfe.FileName.Length == 0 ? true : false);
			zfe.FileSize = unchecked((int)ucsize);
			zfe.FileMonth = mo;
			zfe.FileDay = dy;
			zfe.FileYear = yr;
			zfe.FileHour = hh;
			zfe.FileMinute = mm;
			zfe.CompressedSize = unchecked((int)csiz);
			zfe.CompressionFactor = cfactor;

			m_ZipFileEntries.Add(zfe);

		}

		protected int UZDLLPrint (ref UNZIPCBChar msg, uint x)
		{
			string s = string.Empty;

			if (msg.ch[0] == 0) return 0;
			s = m_Ascii.GetString(msg.ch,0,unchecked((int)x));

			UnZipDLLPrintMessageEventArgs e = new UnZipDLLPrintMessageEventArgs(s);
			OnReceivePrintMessage(e);

			return 0;
		}

		/*
		DLLSERVICE *ServCallBk  = Callback function designed to be used for
						  allowing the application to process Windows messages,
						  or canceling the operation, as well as giving the
						  option of a progress indicator. If this function
						  returns a non-zero value, then it will terminate
						  what it is doing. It provides the application with
						  the name of the name of the archive member it has
						  just processed, as well as it's original size.

		msg.ch = the name of the file being zipped
		x = The size of the file being zipped

		 * */
		protected int UZDLLService (ref UNZIPCBChar fname, uint x)
		{
			string s = string.Empty;
			int i = 0;

			for (i = 0; i <= fname.ch.Length; i ++)
				if (fname.ch[i] == 0) break;
			s = m_Ascii.GetString(fname.ch,0,i);

			//Raise this event
			UnZipDLLServiceMessageEventArgs e = new UnZipDLLServiceMessageEventArgs(m_ZipFileSize, s, unchecked((int)x));
			OnReceiveServiceMessage (e);

			return m_Stop;
		}

		protected short UZDLLPassword (ref UNZIPCBCh p, int n, ref UNZIPCBCh m, ref UNZIPCBCh name)
		{
			if (m_Password == null | m_Password == string.Empty) return 1;

			//clear the byte array
			for (int i = 0; i <= n-1; i ++)
				p.ch[i] = 0;

			m_Ascii.GetBytes(m_Password, 0, m_Password.Length, p.ch, 0);

			return 0;
		}

		protected int UZDLLReplace (ref UNZIPCBChar fname)
		{
			string s = string.Empty;
			int i = 0;

			for (i = 0; i <= fname.ch.Length; i ++)
				if (fname.ch[i] == 0) break;
			s = m_Ascii.GetString(fname.ch,0,i);

			DialogResult rslt = MessageBox.Show("Overwrite [" + s + "]?  Click Cancel to overwrite all.",
																	"Overwrite Confirmation", MessageBoxButtons.YesNoCancel,
																	MessageBoxIcon.Question);
			switch (rslt)
			{
				case DialogResult.No:
					m_ReplaceFileOption = ReplaceFileOptionsEnum.OverwritePrompt;
					break;
				case DialogResult.Yes:
					m_ReplaceFileOption = ReplaceFileOptionsEnum.Overwrite;
					break;
				case DialogResult.Cancel:
					m_ReplaceFileOption = ReplaceFileOptionsEnum.OverwriteAll;
					break;
			}
			return ConvertEnumToInt(m_ReplaceFileOption);
		}

		#endregion

		#region Public Functions

		public int UnZipFiles ()
		{
			int ret = -1;

			//check to see if there is enough information to proceed.
			//Exceptions can be thrown if required data is not passed in
			if (m_ZipFileName == string.Empty) return -1;
			if (m_ExtractionDirectory == string.Empty) return -1;

			//The zip file size, in bytes, is stored in the m_ZipFileSize variable.
			//m_ZipFileCount is the number of files in the zip.  This information
			//is useful for some sort of progress information during unzipping.
			GetZipFileSizeAndCount();

			DCLIST dclist = new DCLIST();
			dclist.ExtractOnlyNewer = ConvertEnumToInt(m_ExtractOnlyNewer);
			dclist.SpaceToUnderscore = ConvertEnumToInt(m_SpaceToUnderScore);
			dclist.PromptToOverwrite = ConvertEnumToInt(m_PromptToOverWrite);
			dclist.fQuiet = ConvertEnumToInt(m_Quiet);
			dclist.ncflag = ConvertEnumToInt(m_WriteStdOut);
			dclist.ntflag = ConvertEnumToInt (m_TestZip);
			dclist.nvflag = ConvertEnumToInt(m_ExtractList);
			dclist.nfflag = ConvertEnumToInt(m_FreshenExisting);
			dclist.nzflag = ConvertEnumToInt(m_DisplayComment);
			dclist.ndflag = ConvertEnumToInt(m_HonorDirectories);
			dclist.noflag = ConvertEnumToInt(m_OverWriteFiles);
			dclist.naflag = ConvertEnumToInt(m_ConvertCR_CRLF);
			dclist.nZIflag = ConvertEnumToInt(m_Verbose);
			dclist.C_flag = ConvertEnumToInt(m_CaseSensitivity);
			dclist.fPrivilege = ConvertEnumToInt(m_Privilege);
			dclist.Zip = m_ZipFileName;
			dclist.ExtractDir = m_ExtractionDirectory;

			USERFUNCTION uf = PrepareUserFunctionCallbackStructure();

			try
			{
				ret = Wiz_SingleEntryUnzip(m_FilesToUnzip.Length, m_FilesToUnzip, m_FilesToExclude.Length,
					m_FilesToExclude, ref dclist, ref uf);
			}
			catch (Exception e)
			{
				MessageBox.Show (e.ToString() + "\r\n" + "Last Win32ErrorCode: " + Marshal.GetLastWin32Error());
				//You can check the meaning of return codes here:
				//http://msdn.microsoft.com/library/default.asp?url=/library/en-us/debug/base/system_error_codes__0-499_.asp
			}

			return ret;
		}

		public void GetZipFileSizeAndCount(ref int size, ref int fileCount)
		{
			GetZipFileSizeAndCount();
			size = m_ZipFileSize;
			fileCount = m_ZipFileCount;
		}

		public ZipFileEntries GetZipFileContents ()
		{
			int ret = 0;

			DCLIST dclist = new DCLIST();
			dclist.nvflag = ConvertEnumToInt(ExtractListEnum.ListContents);
			dclist.Zip = m_ZipFileName;

			USERFUNCTION uf = PrepareUserFunctionCallbackStructure();

			m_ZipFileSize = 0;
			m_ZipFileCount = 0;
			m_ZipFileEntries.Clear();

			//This call will fill the m_ZipFileEntries collection because when the nvflag = ExtractListEnum.ListContents
			//the UZReceiveDLLMessage callback function is called
			try
			{
				ret = Wiz_SingleEntryUnzip(m_FilesToUnzip.Length, m_FilesToUnzip, m_FilesToExclude.Length,
					m_FilesToExclude, ref dclist, ref uf);
			}
			catch(Exception e)
			{
				MessageBox.Show (e.ToString() + "\r\n" + "Last Win32ErrorCode: " + Marshal.GetLastWin32Error());
				//You can check the meaning of return codes here:
				//http://msdn.microsoft.com/library/default.asp?url=/library/en-us/debug/base/system_error_codes__0-499_.asp
			}

			return m_ZipFileEntries;
		}

		public string GetZipFileComment()
		{
			//WORK AROUND:
			//This method provides a work around to setting the nzflag of the DCLIST structure = 1, which instructs
			//the dll to extract the zip file comment.  See the KNOWN ISSUES region at the beginning of this code
			//sample.

			//NOTE:
			//Explanation of Big Endian and Little Endian Architecture
			//http://support.microsoft.com/default.aspx?scid=kb;en-us;102025
			//Bytes in the stream are in Big Endian format.  We have to read the bytes and
			//convert to Little Endian format.  That's what the GetLittleEndianByteOrder function
			//does.

			const int CENTRALRECORDENDSIGNATURE = 0x06054b50;
			const int CENTRALRECORDENDSIZE = 22;

			string comment = string.Empty;
			ASCIIEncoding ae = new ASCIIEncoding();

			if (m_ZipFileName == null | m_ZipFileName == string.Empty) return string.Empty;

			try
			{
				FileStream fs = File.OpenRead(m_ZipFileName);
				long pos = fs.Length - CENTRALRECORDENDSIZE;

				while (GetLittleEndianByteOrder(fs,4) != CENTRALRECORDENDSIGNATURE)
					fs.Seek(pos--, SeekOrigin.Begin);


				int diskNumber = GetLittleEndianByteOrder(fs,2);						/* number of this disk */
				int startingDiskNum = GetLittleEndianByteOrder(fs,2);				/* number of the starting disk */
				int entriesOnrThisDisk = GetLittleEndianByteOrder(fs,2);			/* entries on this disk */
				int totalEntries = GetLittleEndianByteOrder(fs,2);						/* total number of entries */
				int centralDirectoryTotalSize = GetLittleEndianByteOrder(fs,4);	/* size of entire central directory */
				int offsetOfCentralDirectory = GetLittleEndianByteOrder(fs,4);	/* offset of central on starting disk */
				//This is what we really want here
				int commentSize = GetLittleEndianByteOrder(fs,2);					/* length of zip file comment */


				byte[] zipFileComment = new byte[commentSize];
				fs.Read(zipFileComment, 0, zipFileComment.Length);

				comment =  ae.GetString(zipFileComment, 0, zipFileComment.Length);
				fs.Close();
			}
			catch (Exception e)
			{
				throw new Exception(e.Message);
			}
			return comment;
		}

		public void Stop ()
		{
			//m_Stop gets returned from the UZDLLService callback.
			//A value of 1 means abort processing.
			m_Stop = 1;
		}

		#endregion

		#region Private Functions

		private int ConvertEnumToInt (System.Enum obj)
		{
			return Convert.ToInt32(obj);
		}

		private void GetZipFileSizeAndCount()
		{
			int ret = 0;

			DCLIST dclist = new DCLIST();
			dclist.nvflag = ConvertEnumToInt(ExtractListEnum.ListContents);
			dclist.Zip = m_ZipFileName;
			dclist.ExtractDir = m_ExtractionDirectory;

			USERFUNCTION uf = PrepareUserFunctionCallbackStructure();

			//Reset these variables
			m_ZipFileSize = 0;
			m_ZipFileCount = 0;

			ret = Wiz_SingleEntryUnzip(m_FilesToUnzip.Length, m_FilesToUnzip, m_FilesToExclude.Length,
														m_FilesToExclude, ref dclist, ref uf);
		}


		private USERFUNCTION PrepareUserFunctionCallbackStructure()
		{
			USERFUNCTION uf = new USERFUNCTION();
			uf.UZDLLPrnt = new UZDLLPrintCallback(UZDLLPrint);
			uf.UZDLLSND = 0; //Not supported
			uf.UZDLLREPLACE = new UZDLLReplaceCallback(UZDLLReplace);
			uf.UZDLLPASSWORD = new UZDLLPasswordCallback(UZDLLPassword);
			uf.UZDLLMESSAGE = new UZReceiveDLLMessageCallback(UZReceiveDLLMessage);
			uf.UZDLLSERVICE = new UZDLLServiceCallback(UZDLLService);

			return uf;
		}

		private int GetLittleEndianByteOrder(FileStream fs, int len)
		{
			int result = 0;
			int n = 0;
			int [] byteArr = new int[len];

			//Pull the bytes from the stream
			for(n=0;n<len;n++)
				byteArr[n] = fs.ReadByte();

			//Left shift the bytes to get a resulting number in little endian format
			for(n=0;n<byteArr.Length;n++)
				result += (byteArr[n] << (n*8));

			return result;
		}

		#endregion
	}
}
