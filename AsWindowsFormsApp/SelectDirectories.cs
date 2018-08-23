using System;
using System.IO;
using System.Windows.Forms;

namespace AsWindowsFormsApp
{
    public partial class SelectDirectories : Form
    {
        public SelectDirectories()
        {
            InitializeComponent();

            // Prepopulate the list with local driver letters.
            string [] drives = Directory.GetLogicalDrives();
            foreach (string drive in drives)
            {
                this.listViewFolders.Items.Add(drive);
            }
        }

        /// <summary>
        /// Gets all of the selected directories.
        /// </summary>
        public string [] Directories
        {
            get
            {
                ListView.CheckedListViewItemCollection items = this.listViewFolders.CheckedItems;
                string[] dirs = new string[items.Count];
                int x = 0;
                foreach (ListViewItem item in items)
                {
                    dirs[x] = item.Text;
                    x++;
                }

                return dirs;
            }
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void buttonOK_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void buttonAdd_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog folderDlg = new FolderBrowserDialog();
            folderDlg.SelectedPath = this.textAddFolderPath.Text;
            if (DialogResult.OK == folderDlg.ShowDialog())
            {
                ListViewItem item = this.listViewFolders.Items.Add(folderDlg.SelectedPath);
                item.Checked = true;
                this.textAddFolderPath.Text = "";
            }
        }
    }
}
