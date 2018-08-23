using System;
using System.Windows.Forms;
using System.IO;

namespace AsWindowsFormsApp
{
    public class DirectoryNode
    {
        /// <summary>
        /// Name of this folder.
        /// </summary>
        public string Name { get; private set; }

        /// <summary>
        /// Full path of this folder.
        /// </summary>
        public string MyPath { get; private set; }

        /// <summary>
        /// Size in bytes of the files in this folder.
        /// </summary>
        public UInt64 SizeFiles { get; private set; }

        /// <summary>
        /// Size in bytes of files in all subfolders of this folder.
        /// </summary>
        public UInt64 SizeFolders { get; private set; }

        /// <summary>
        /// Total size in bytes of this folder and all subfolders.
        /// </summary>
        public UInt64 SizeTotal
        {
            get
            {
                return this.SizeFiles + this.SizeFolders;
            }
        }

        public bool SizeKnown { get; private set; }

        /// <summary>
        /// Message of any exception that occurred while finding the size of this folder.
        /// </summary>
        public string ExceptionText
        {
            get
            {
                string str = "";
                if (null != this.myException)
                {
                    str = this.myException.Message;
                }

                return str;
            }
        }

        private MainForm form;
        private Exception myException;
        private TreeNode TreeNode { get; set; }

        public DirectoryNode(MainForm form, string parentPath, string name, TreeNodeCollection treeNodeCollection)
        {
            if (null == form)
            {
                throw new ArgumentNullException("form", "DirectoryNode requires form parameter.");
            }

            if (string.IsNullOrEmpty(name))
            {
                throw new ArgumentNullException("name", "DirectoryNode requires name parameter.");
            }

            this.SizeKnown = false;
            this.form = form;
            this.Name = name;
            if (string.IsNullOrEmpty(parentPath))
            {
                this.MyPath = name;
            }
            else
            {
                this.MyPath = Path.Combine(parentPath, name);
            }

            if (!Directory.Exists(this.MyPath))
            {
                throw new ApplicationException(string.Format("Directory {0} does not exist", this.MyPath));
            }

            this.TreeNode = form.AddTreeFolder(this.MyPath, treeNodeCollection, this);
        }

        public void FindSize()
        {
            try
            {
                UInt64 sizeFiles = 0;
                foreach (string file in Directory.GetFiles(this.MyPath))
                {
                    FileInfo fi = new FileInfo(file);
                    sizeFiles += (ulong) fi.Length;
                }

                this.SizeFiles = sizeFiles;

                this.form.UpdateTotalSize(sizeFiles);

                // Don't sort the directories as we find their sizes. Doing so appears to
                // cause the entire tree to be recreated from scratch.  Flickering that
                // results is unacceptable.
                // May need to look into manually ordering the nodes so as to avoid 
                // unnecessary
                //this.form.SortTree();

                foreach (string directory in Directory.GetDirectories(this.MyPath))
                {
                    DirectoryNode node = new DirectoryNode(this.form, this.MyPath, Path.GetFileName(directory), this.TreeNode.Nodes);
                    node.FindSize();
                    this.SizeFolders = this.SizeFolders + node.SizeTotal;
                }

                this.SizeKnown = true;
            }
            catch (Exception ex)
            {
                this.myException = ex;
            }
        }

        public override string ToString()
        {
            return this.MyPath;
        }
    }
}
