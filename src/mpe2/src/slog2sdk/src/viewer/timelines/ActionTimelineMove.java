/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package viewer.timelines;

import java.awt.*;
import java.awt.event.*;
import java.net.*;
import javax.swing.*;
import javax.swing.tree.*;
import java.util.*;

import viewer.common.Dialogs;
import viewer.common.TopWindow;

public class ActionTimelineMove implements ActionListener
{
    private TimelineToolBar    toolbar;
    private YaxisTree          tree;
    private DefaultTreeModel   tree_model;

    public ActionTimelineMove( TimelineToolBar in_toolbar, YaxisTree in_tree )
    {
        toolbar      = in_toolbar;
        tree         = in_tree;
        tree_model   = (DefaultTreeModel) tree.getModel();
    }

    public void actionPerformed( ActionEvent event )
    {
        Debug.displayLine( "Action for Move Timeline button" );
        Enumeration[]     enum_paths;
        Enumeration       paths;
        boolean[]         isBufferedPathExpanded;
        TreePath          selected_path, parent_path;
        TreePath          old_path, new_path, prefix_path;
        TreePath[]        child_paths;
        MutableTreeNode   selected, parent, child;
        int               insertion_idx;
        int               idx;

        if ( tree.getSelectionCount() != 1 ) {
            Dialogs.error( TopWindow.Timeline.getWindow(),
                "ONE selected tree node is needed as an insertion point!" );
            return;
        }

        selected_path  = tree.getSelectionPath();
        child_paths    = tree.getFromCutAndPasteBuffer();
        
        if ( child_paths == null || child_paths.length < 1 ) {
            Dialogs.warn( TopWindow.Timeline.getWindow(),
                          "Nothing has been marked for relocation!" );
            return;
        }

        if ( !tree.isPathLevelSameAsThatOfCutAndPasteBuffer( selected_path ) ) {
            Dialogs.error( TopWindow.Timeline.getWindow(),
                           "The level of insertion point has "
                         + "different level than that of marked timelines. "
                         + "Select another insertion point!" );
            return;
        }

        // Save the expansion pattern of the marked timelines
        // Then collapse any marked timelines which are expanded
        enum_paths = new Enumeration[ child_paths.length ];
        isBufferedPathExpanded = new boolean[ child_paths.length ];
        for ( idx = 0; idx < child_paths.length; idx++ ) {
            enum_paths[ idx ] = tree.getExpandedDescendants(
                                child_paths[ idx ] );
            Debug.println( "action_timeline_move(): emum_paths[" + idx + "] = "
                         + enum_paths[ idx ] );
            if ( tree.isExpanded( child_paths[ idx ] ) ) {
                isBufferedPathExpanded[ idx ] = true;
                tree.collapsePath( child_paths[ idx ] );
            }
            else
                isBufferedPathExpanded[ idx ] = false;
        }

        // Remove the marked timelines from the tree
        for ( idx = 0; idx < child_paths.length; idx++ ) {
            child = (MutableTreeNode) child_paths[ idx ].getLastPathComponent();
            tree_model.removeNodeFromParent( child );
            Debug.displayLine( "\tCut " + child );
        }

        // Add the marked timelines to the insertion point
        parent_path = selected_path.getParentPath();
        Debug.println( "action_timeline_move(): parent_path = " + parent_path );
        parent   = (MutableTreeNode) parent_path.getLastPathComponent();
        selected = (MutableTreeNode) selected_path.getLastPathComponent();
        insertion_idx = ( (DefaultMutableTreeNode) parent ).getIndex( selected )
                      + 1;
        for ( idx = 0; idx < child_paths.length; idx++ ) {
            child = (MutableTreeNode) child_paths[ idx ].getLastPathComponent();
            tree_model.insertNodeInto( child, parent, insertion_idx + idx );
            Debug.displayLine( "\tPaste " + child + " next to " + selected );
            // Restore the expansion pattern of marked timelines
            prefix_path = parent_path.pathByAddingChild( child );
            if ( isBufferedPathExpanded[ idx ] )
                tree.expandPath( prefix_path );
            if ( enum_paths[ idx ] != null ) {
                paths = enum_paths[ idx ];
                while ( paths.hasMoreElements() ) {
                    old_path = (TreePath) paths.nextElement();
                    Debug.print( "action_timeline_move(): " + old_path );
                    new_path = this.getReplacedPath( prefix_path, old_path );
                    Debug.println( " is restored as " + new_path );
                    tree.expandPath( new_path );
                }
            }
        }

        // Clear up the marked timelines in the buffer
        tree.clearCutAndPasteBuffer();

        // Update leveled_paths[]
        tree.update_leveled_paths();

        // Set toolbar buttons to reflect status
        toolbar.mark_btn.setEnabled( true );
        toolbar.move_btn.setEnabled( false );
        toolbar.delete_btn.setEnabled( false );
        toolbar.expand_btn.setEnabled( tree.isLevelExpandable() );
        toolbar.collapse_btn.setEnabled( tree.isLevelCollapsable() );
        toolbar.commit_btn.setEnabled( true );
    }

    private TreePath getReplacedPath( TreePath prefix_path, TreePath old_path )
    {
        TreePath                new_path;
        MutableTreeNode         common_node;
        DefaultMutableTreeNode  last_node;
        Enumeration             nodes;

        common_node = (MutableTreeNode) prefix_path.getLastPathComponent();
        new_path    = prefix_path.getParentPath();
        last_node   = (DefaultMutableTreeNode) old_path.getLastPathComponent();
        nodes       = last_node.pathFromAncestorEnumeration( common_node );
        while ( nodes.hasMoreElements() )
            new_path = new_path.pathByAddingChild( nodes.nextElement() );

        return new_path;
    }
}
