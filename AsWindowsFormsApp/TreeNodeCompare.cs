using System;
using System.Collections;
using System.Windows.Forms;

namespace AsWindowsFormsApp
{
    /// <summary>
    /// Comparer for sorting directory nodes in the tree view.
    /// </summary>
    class TreeNodeCompare : IComparer
    {
        public int Compare(object oLeft, object oRight)
        {
            int result;
            TreeNode leftNode = oLeft as TreeNode;
            TreeNode rightNode = oRight as TreeNode;
            DirectoryNode leftDirNode = leftNode.Tag as DirectoryNode;
            DirectoryNode rightDirNode = rightNode.Tag as DirectoryNode;

            if (null == rightDirNode)
            {
                result = 1;
            }
            else if (leftDirNode.SizeTotal < rightDirNode.SizeTotal)
            {
                result = 1;
            }
            else if (leftDirNode.SizeTotal > rightDirNode.SizeTotal)
            {
                result = -1;
            }
            else
            {
                result = 0;
            }

            return result;
        }
    }
}
