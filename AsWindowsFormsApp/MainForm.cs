using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.Threading;
using System.Windows.Forms;

namespace AsWindowsFormsApp
{
    delegate TreeNode AddTreeFolderDeligate(string folderPath, TreeNodeCollection treeNodeCollection, DirectoryNode dirNode);
    delegate void SortTreeDeligate();
    delegate void PopulateTreeCompletedDeligate(DirectoryNode directoryNode);
    delegate void UpdateTotalSizeDeligate(UInt64 size);

    public partial class MainForm : Form
    {
        /// <summary>
        /// Contains left and right sides in client area where a column can have its text.
        /// </summary>
        private class TextBounds
        {
            public int Left { get; private set; }
            public int Right { get; private set; }

            public TextBounds()
            {
                this.Left = 0;
                this.Right = 0;
            }

            public void SetBounds(int left, int right)
            {
                this.Left = left;
                this.Right = right;
            }

            public override string ToString()
            {
                return string.Format("Left: {0} Right: {1}", this.Left, this.Right);
            }
        }

        bool shuttingDown = false;

        Brush eraserBrush;
        Brush testBrush;
        Pen solidBlackPen = new Pen(Color.Black, 1);
        Pen dottedBlackPen = new Pen(Color.Black, 1);
        Image fileClosedImage;
        Image fileOpenImage;
        ImageList treeViewImageList = new ImageList();
        TextBounds folderColumn = new TextBounds();
        TextBounds percentColumn = new TextBounds();
        TextBounds sizeColumn = new TextBounds();
        StringFormat stringFormatNoWrap = new StringFormat(StringFormatFlags.NoWrap);
        StringFormat stringFormatNoWrapCenter = new StringFormat(StringFormatFlags.NoWrap);
        StringFormat stringFormatNoWrapNumber = new StringFormat(StringFormatFlags.NoWrap);

        private ulong TotalSize { get; set; }

        private Dictionary<string, Thread> threads = new Dictionary<string, Thread>();

        public MainForm()
        {
            this.TotalSize = 0;
            this.dottedBlackPen.DashPattern = new float[] { 1.0F, 1.0F, 1.0F, 1.0F };
            InitializeComponent();
            this.treeView.FullRowSelect = true;
            this.eraserBrush = new SolidBrush(treeView.BackColor);
            this.testBrush = new SolidBrush(Color.Red);
            this.stringFormatNoWrap.Trimming = StringTrimming.EllipsisCharacter;
            this.stringFormatNoWrapCenter.Alignment = StringAlignment.Center;
            this.stringFormatNoWrapCenter.Trimming = StringTrimming.EllipsisCharacter;
            this.stringFormatNoWrapNumber.Alignment = StringAlignment.Far;
            this.stringFormatNoWrapNumber.Trimming = StringTrimming.EllipsisCharacter;
            this.fileClosedImage = new Bitmap("FileClosed.bmp");
            this.fileOpenImage = new Bitmap("FileOpen.bmp");

            // Not actually using this image list. Adding an image lists causes the control to leave 
            // correct amount of space between graphics it draws and owner drawn text area.
            // Doing this so I don't have to draw all the connecting line graphics.
            this.treeView.ImageList = new ImageList();
            this.treeView.ImageList.Images.Add(this.fileClosedImage);
            this.treeView.ImageList.Images.Add(this.fileOpenImage);
            this.SetTreeViewWindowPos();
        }

        private void UpdateStatusBar()
        {
            this.toolStripStatusLabel_ActiveThreads.Text = string.Format("{0} Threads Running", this.threads.Count);
        }

        private void StopBackgroundThreads()
        {
            foreach (Thread th in threads.Values)
            {
                ThreadState state = th.ThreadState;
                if (state != ThreadState.Aborted && state != ThreadState.AbortRequested && state != ThreadState.Stopped)
                {
                    th.Abort();
                }

                th.Join();
            }

            threads.Clear();
        }

        /// <summary>
        /// Worker thread entry point.
        /// </summary>
        /// <param name="dir">Full path of root directory to start with.</param>
        private void PopulateTreeTh(object dir)
        {
            string directory = dir as string;
            DirectoryNode rootDirectoryNode = new DirectoryNode(this, null, directory, this.treeView.Nodes);
            rootDirectoryNode.FindSize();
            PopulateTreeCompleted(rootDirectoryNode);
        }

        /// <summary>
        /// Starts a worker thread that will go calculate size of all subfolders.
        /// </summary>
        /// <param name="directories">Full path of root directories to start with.</param>
        public void PopulateTree(string [] directories)
        {
            if (null == directories)
            {
                throw new ArgumentNullException("directories", "PopulateTree requires the directories parameter.");
            }

            foreach (string directory in directories)
            {
                Thread th = new Thread(new ParameterizedThreadStart(PopulateTreeTh));
                th.Start(directory);
                this.threads.Add(directory, th);
                UpdateStatusBar();
            }
        }

        /// <summary>
        /// Adds a folder to the tree view.
        /// </summary>
        /// <param name="folderPath"></param>
        /// <param name="treeNodeCollection">Collection of nodes which are siblings of the node to be added.</param>
        /// <param name="dirNode">DirectoryNode that represents the folder being added.</param>
        /// <returns>TreeNode which child nodes can be added to.</returns>
        public TreeNode AddTreeFolder(string folderPath, TreeNodeCollection treeNodeCollection, DirectoryNode dirNode)
        {
            TreeNode treeNode = null;
            if (this.treeView.InvokeRequired)
            {
                AddTreeFolderDeligate addTreeFolderDeligate = new AddTreeFolderDeligate(AddTreeFolder);
                treeNode = this.Invoke(addTreeFolderDeligate, new object[] { folderPath, treeNodeCollection, dirNode }) as TreeNode;
            }
            else
            {
                treeNode = treeNodeCollection.Add(folderPath);
                treeNode.Tag = dirNode;
            }

            return treeNode;
        }

        /// <summary>
        /// Adds a folders file sizes to the total over all size.
        /// </summary>
        /// <param name="size">Size in bytes of all files in a folder.</param>
        public void UpdateTotalSize(UInt64 size)
        {
            if (this.treeView.InvokeRequired)
            {
                UpdateTotalSizeDeligate updateTotalSizeDeligate = new UpdateTotalSizeDeligate(UpdateTotalSize);
                this.Invoke(updateTotalSizeDeligate, new object[] { size });
            }
            else
            {
                this.TotalSize += size;
            }
        }

        /// <summary>
        /// Sorts the tree by size.
        /// </summary>
        /// <remarks>
        /// Appears to remove and add back all nodes in the entire tree. Is recursive, not just
        /// at the node level.
        /// </remarks>
        public void SortTree()
        {
            if (treeView.InvokeRequired)
            {
                SortTreeDeligate sortTreeDeligate = new SortTreeDeligate(SortTree);
                this.Invoke(sortTreeDeligate);
            }
            else
            {
                this.treeView.Sort();
            }
        }

        /// <summary>
        /// Signals thread completion.
        /// </summary>
        /// <param name="directoryNode">Root directory that was added.</param>
        public void PopulateTreeCompleted(DirectoryNode directoryNode)
        {
            if (treeView.InvokeRequired)
            {
                PopulateTreeCompletedDeligate populateTreeCompletedDeligate = new PopulateTreeCompletedDeligate(PopulateTreeCompleted);
                this.Invoke(populateTreeCompletedDeligate, new object[] { directoryNode });
            }
            else
            {
                Thread th = this.threads[directoryNode.MyPath];
                this.threads.Remove(directoryNode.MyPath);
                this.SortTree();
                UpdateStatusBar();
            }
        }

        private void MainForm_Shown(object sender, EventArgs e)
        {
            this.treeView.TreeViewNodeSorter = new TreeNodeCompare();
            this.treeView.BeginUpdate();
            this.SetTreeViewWindowPos();
            this.SetColumnPos();
            this.treeView.EndUpdate();

            this.TotalSize = 0;

            SelectDirectories selectDirectories = new SelectDirectories();
            DialogResult res = selectDirectories.ShowDialog(this);
            if (DialogResult.OK == res)
            {
                PopulateTree(selectDirectories.Directories);
            }
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            AboutBox aboutBox = new AboutBox();
            aboutBox.ShowDialog(this);
        }

        private void ExitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void MainForm_Resize(object sender, EventArgs e) => SetTreeViewWindowPos();

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            this.shuttingDown = true;
            this.StopBackgroundThreads();
        }

        private void btnFolder_Move(object sender, EventArgs e)
        {
            this.SetColumnPos();
        }

        private void btnPercent_Move(object sender, EventArgs e)
        {
            this.SetColumnPos();
        }

        private void btnSize_Move(object sender, EventArgs e)
        {
            this.SetColumnPos();
        }

        private void SetTreeViewWindowPos()
        {
            Rectangle rcClient = ClientRectangle;
            int topControlHeight = menu.Height + splitContainer1.Height;
            this.treeView.SetBounds(
                rcClient.Left,
                rcClient.Top + topControlHeight,
                rcClient.Right,
                rcClient.Bottom - status.Height - topControlHeight);
        }

        private void btnFolder_SizeChanged(object sender, EventArgs e)
        {
            this.SetColumnPos();
            this.RedrawTreeView();
        }

        private void btnPercent_SizeChanged(object sender, EventArgs e)
        {
            this.SetColumnPos();
            this.RedrawTreeView();
        }

        private void btnSize_SizeChanged(object sender, EventArgs e)
        {
            this.SetColumnPos();
            this.RedrawTreeView();
        }

        /// <summary>
        /// Convert client coordiates of the button to client coordiantes of this form.
        /// </summary>
        /// <param name="control">Control to calculate form client coordiantes for.</param>
        /// <param name="textBounds">Calculated client coordiantes will be stored in textBounds.</param>
        /// <returns>Point object that has the button's left and right sides stored in X and Y variable.</returns>
        private void ConvertContainedWinBoundsToFormClientCoordiantes(Control control, TextBounds textBounds)
        {
            Point ptControlClientLocation = new Point(control.ClientRectangle.Left, control.ClientRectangle.Right);
            Point ptControlScreenLocation = control.PointToScreen(ptControlClientLocation);
            Point ptControlFormClientLocation = this.PointToClient(ptControlScreenLocation);
            textBounds.SetBounds(ptControlFormClientLocation.X, ptControlFormClientLocation.X + (control.ClientRectangle.Right - control.ClientRectangle.Left));
        }

        /// <summary>
        /// Update the bounding columns for display text.
        /// </summary>
        private void SetColumnPos()
        {
            this.ConvertContainedWinBoundsToFormClientCoordiantes(btnFolder, folderColumn);
            this.ConvertContainedWinBoundsToFormClientCoordiantes(btnPercent, percentColumn);
            this.ConvertContainedWinBoundsToFormClientCoordiantes(btnSize, sizeColumn);
        }

        private void RedrawTreeView()
        {
            this.treeView.Invalidate();
            this.treeView.Update();
        }

        /// <summary>
        /// Draw folder information.
        /// </summary>
        /// <remarks>
        /// Implementing double buffering here.  1) There is a flickering that happens in the area where tree view 
        /// thinks the text is drawn. Which occurs even when the node is a null string. The control is erasing 
        /// background, and not found anyway to prevent the background erasing from happening.  2) Have to draw 
        /// our own icons.  The control allows only for 2 icons. Normal and selected.  Want to draw a diffent 
        /// icon when the node is expanded.  3) Would like to use OwnerDrawText, but there is too much flickering.
        /// In the area where control thinks the text is.
        /// </remarks>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void treeView_DrawNode(object sender, DrawTreeNodeEventArgs e)
        {
            if (e.Node.IsVisible && !this.shuttingDown)
            {
                Font nodeFont = e.Node.NodeFont;
                if (nodeFont == null)
                {
                    nodeFont = ((TreeView)sender).Font;
                }

                Rectangle rcNode = e.Node.Bounds;
                Rectangle rc = new Rectangle(0, 0, this.treeView.Width, rcNode.Height);
                using (Bitmap memBmp = new Bitmap(this.treeView.Width, rc.Height, e.Graphics))
                {
                    using (Graphics inMemoryGraphic = Graphics.FromImage(memBmp))
                    {
                        inMemoryGraphic.FillRectangle(this.eraserBrush, rc);

                        rc.Width = percentColumn.Left - 5 - rcNode.X - 5;
                        DirectoryNode dirNode = e.Node.Tag as DirectoryNode;

                        // Folder
                        inMemoryGraphic.DrawString(dirNode.Name, nodeFont, Brushes.Black, rc, this.stringFormatNoWrap);
                        //inMemoryGraphic.FillRectangle(this.testBrush, rc);

                        // Percent
                        rc.X = rc.X + rc.Width + 15;
                        rc.Width = percentColumn.Right - percentColumn.Left - 10;
                        string percentText = "Percent";
                        if (!string.IsNullOrEmpty(dirNode.ExceptionText))
                        {
                            percentText = dirNode.ExceptionText;
                        }
                        else
                        {
                            bool sizeKnown = dirNode.SizeKnown;
                            double nodeSize = dirNode.SizeTotal;
                            double totalSize = this.TotalSize;
                            double percent;
                            if (0 == totalSize)
                            {
                                percent = 0;
                            }
                            else
                            {
                                percent = nodeSize / totalSize;
                            }

                            percentText = string.Format("{0}", percent.ToString("#0.##%", CultureInfo.InvariantCulture));
                            if (!sizeKnown)
                            {
                                percentText += " - Computing";
                            }
                        }

                        inMemoryGraphic.DrawString(percentText, nodeFont, Brushes.Black, rc, this.stringFormatNoWrapCenter);
                        //inMemoryGraphic.FillRectangle(this.testBrush, rc);

                        // Size
                        rc.X = rc.X + rc.Width + 15;
                        rc.Width = sizeColumn.Right - sizeColumn.Left - 15;
                        inMemoryGraphic.DrawString(dirNode.SizeTotal.ToString(), nodeFont, Brushes.Black, rc, this.stringFormatNoWrapNumber);
                        //inMemoryGraphic.FillRectangle(this.testBrush, rc);

                        e.Graphics.DrawImage(memBmp, rcNode.X, rcNode.Y);
                    }
                }

                // Draw in connecting lines and icon.  Draw as seperate bitmap to the left of text.
                int bmpWidth = rcNode.X - (e.Node.Level * this.treeView.Indent);
                using (Bitmap memBmp = new Bitmap(bmpWidth, rc.Height, e.Graphics))
                {
                    using (Graphics inMemoryGraphic = Graphics.FromImage(memBmp))
                    {
                        Point bmpLocation = new Point(bmpWidth - 19, 0);
                        if (e.Node.IsExpanded)
                        {
                            inMemoryGraphic.DrawImage(this.fileOpenImage, bmpLocation);
                        }
                        else
                        {
                            inMemoryGraphic.DrawImage(this.fileClosedImage, bmpLocation);
                        }

                        Point center = new Point(bmpLocation.X - 12, rcNode.Height / 2);
                        // Draw a box if the node has children.
                        if (e.Node.Nodes.Count > 0)
                        {
                            Rectangle rcBox = new Rectangle(center.X - 3, center.Y - 3, 6, 6);
                            inMemoryGraphic.DrawRectangle(this.solidBlackPen, rcBox);

                            // If the node is collapsed, draw a vertical line in the box.
                            if (!e.Node.IsExpanded)
                            {
                                Point vertTop = new Point(center.X, center.Y - 2);
                                Point vertBottom = new Point(center.X, center.Y + 2);
                                inMemoryGraphic.DrawLine(this.solidBlackPen, vertTop, vertBottom);
                            }
                            else
                            {   // Draw a dotted line from the box down.
                                Point boxBottom = new Point(center.X, center.Y + 4);
                                Point bottom = new Point(center.X, memBmp.Height);
                                inMemoryGraphic.DrawLine(this.dottedBlackPen, boxBottom, bottom);
                            }

                            // Always draw line going across the center of the box.
                            Point horzLeft = new Point(center.X - 2, center.Y);
                            Point horzRight = new Point(center.X + 2, center.Y);
                            inMemoryGraphic.DrawLine(this.solidBlackPen, horzLeft, horzRight);

                            // Draw line from box to the icon.
                            horzLeft.X = rcBox.X + rcBox.Width + 1;
                            horzRight.X = bmpLocation.X - 1;
                            inMemoryGraphic.DrawLine(this.dottedBlackPen, horzLeft, horzRight);

                            // If the node has a parent, draw to left.  Will draw up later in a different BMP.
                            if (null != e.Node.Parent)
                            {
                                horzLeft.X = 0;
                                horzRight.X = rcBox.X - 1;
                                inMemoryGraphic.DrawLine(this.dottedBlackPen, horzLeft, horzRight);
                            }
                        }
                        else
                        {
                            // If the node has a parent, draw to left.  Will draw up later in a different BMP.
                            if (null != e.Node.Parent)
                            {
                                Point horzLeft = new Point(0, center.Y);
                                Point horzRight = new Point(bmpLocation.X - 1, center.Y);
                                inMemoryGraphic.DrawLine(this.dottedBlackPen, horzLeft, horzRight);
                            }
                        }

                        e.Graphics.DrawImage(memBmp, rcNode.X - bmpWidth, rcNode.Y);
                    }
                }

                // Draw lines that connect up to the parent's box.
                if (null != e.Node.Parent)
                {
                    using (Bitmap memBmp = new Bitmap(this.treeView.Indent, rc.Height, e.Graphics))
                    {
                        using (Graphics inMemoryGraphic = Graphics.FromImage(memBmp))
                        {
                            Rectangle rcLines = new Rectangle(0, 0, memBmp.Width, memBmp.Height);
                            Point centerLines = new Point(memBmp.Width / 2, memBmp.Height / 2);
                            centerLines.X++;
                            Point rightLines = new Point(memBmp.Width, centerLines.Y);
                            Point topLines = new Point(centerLines.X, 0);

                            inMemoryGraphic.DrawLine(this.dottedBlackPen, centerLines, rightLines);
                            inMemoryGraphic.DrawLine(this.dottedBlackPen, centerLines, topLines);
                            //inMemoryGraphic.FillRectangle(this.testBrush, rcLines);

                            // Extend the line down if this node has a sibbling bellow.
                            if (null != e.Node.NextNode)
                            {
                                Point bottomLines = new Point(centerLines.X, memBmp.Height);
                                inMemoryGraphic.DrawLine(this.dottedBlackPen, centerLines, bottomLines);
                            }

                            memBmp.SetPixel(centerLines.X, 0, Color.Black);

                            e.Graphics.DrawImage(memBmp, (e.Node.Level - 1) * this.treeView.Indent, rcNode.Y);
                        }
                    }
                }

                // When the node has siblings and its children are expanded, need to extend the down line to the sibling.
                if (null != e.Node.Parent && e.Node.IsExpanded && e.Node.Parent.Nodes.Count > 0 && null != e.Node.NextNode)
                {
                    TreeNode nextSibling = e.Node.NextNode;
                    int height = nextSibling.Bounds.Y - e.Node.Bounds.Y;
                    using (Bitmap memBmp = new Bitmap(this.treeView.Indent, height, e.Graphics))
                    {
                        using (Graphics inMemoryGraphic = Graphics.FromImage(memBmp))
                        {
                            Point centerTop = new Point(memBmp.Width / 2, 0);
                            centerTop.X++;
                            Point centerBottom = new Point(centerTop.X, memBmp.Height - 1);
                            inMemoryGraphic.DrawLine(this.dottedBlackPen, centerTop, centerBottom);
                            //inMemoryGraphic.FillRectangle(this.testBrush, 0, 0, memBmp.Width, memBmp.Height);

                            e.Graphics.DrawImage(memBmp, (e.Node.Level - 1) * this.treeView.Indent, e.Node.Bounds.Y + e.Node.Bounds.Height);
                        }
                    }
                }
            }
        }

        private void newSearchToolStripMenuItem_Click(object sender, EventArgs e)
        {
            this.StopBackgroundThreads();
            this.UpdateStatusBar();
            this.treeView.Nodes.Clear();

            this.TotalSize = 0;

            SelectDirectories selectDirectories = new SelectDirectories();
            DialogResult res = selectDirectories.ShowDialog(this);
            if (DialogResult.OK == res)
            {
                PopulateTree(selectDirectories.Directories);
            }
        }
    }
}
