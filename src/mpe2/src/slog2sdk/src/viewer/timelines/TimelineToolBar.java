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
import javax.swing.*;
import javax.swing.event.*;
import java.util.*;
import java.net.URL;

import viewer.common.Const;

public class TimelineToolBar extends JToolBar
                             implements ToolBarStatus
{
    private ViewportTimeYaxis       canvas_vport;
    private JScrollBar              y_scrollbar;
    private YaxisTree               y_tree;
    private YaxisMaps               y_maps;
    private ScrollbarTime           time_scrollbar;
    private ModelTime               time_model;

    public  JButton                 mark_btn;
    public  JButton                 move_btn;
    public  JButton                 delete_btn;
    // public  JButton                 redo_btn;
    // public  JButton                 undo_btn;
    // public  JButton                 remove_btn;

    public  JButton                 up_btn;
    public  JButton                 down_btn;

    public  JButton                 expand_btn;
    public  JButton                 collapse_btn;
    public  JButton                 commit_btn;

    public  JButton                 backward_btn;
    public  JButton                 forward_btn;

    public  JButton                 zoomUndo_btn;
    public  JButton                 zoomOut_btn;
    public  JButton                 zoomHome_btn;
    public  JButton                 zoomIn_btn;
    public  JButton                 zoomRedo_btn;

    public  JButton                 searchBack_btn;
    public  JButton                 searchInit_btn;
    public  JButton                 searchFore_btn;

    public  JButton                 refresh_btn;
    public  JButton                 print_btn;
    public  JButton                 stop_btn;

    private String                  img_path = "/images/";

    public TimelineToolBar( ViewportTimeYaxis canvas_viewport,
                            JScrollBar yaxis_scrollbar,
                            YaxisTree yaxis_tree, YaxisMaps yaxis_maps,
                            ScrollbarTime a_time_scrollbar,
                            ModelTime a_time_model )
    {
        super();
        canvas_vport     = canvas_viewport;
        y_scrollbar      = yaxis_scrollbar;
        y_tree           = yaxis_tree;
        y_maps           = yaxis_maps;
        time_scrollbar   = a_time_scrollbar;
        time_model       = a_time_model;
        this.addButtons();
        canvas_vport.setToolBarStatus( this );
    }

    public void init()
    {
        this.initAllButtons();
    }

    protected URL getURL( String filename )
    {
        URL url = null;

        url = getClass().getResource( filename );

        return url;
    }

    private void addButtons()
    {
        Dimension  mini_separator_size;
        URL        icon_URL;

        mini_separator_size = new Dimension( 5, 5 );

        icon_URL = getURL( img_path + "Up24.gif" );
        if ( icon_URL != null )
            up_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            up_btn = new JButton( "Up" );
        up_btn.setToolTipText( "scroll Upward half a screen" );
        // up_btn.setPreferredSize( btn_dim );
        up_btn.addActionListener( new ActionVportUp( y_scrollbar ) );
        up_btn.setMnemonic( KeyEvent.VK_UP );
        super.add( up_btn );

        icon_URL = getURL( img_path + "Down24.gif" );
        if ( icon_URL != null )
            down_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            down_btn = new JButton( "Down" );
        down_btn.setToolTipText( "scroll Downward half a screen" );
        down_btn.setMnemonic( KeyEvent.VK_DOWN );
        // down_btn.setPreferredSize( btn_dim );
        down_btn.addActionListener( new ActionVportDown( y_scrollbar ) );
        super.add( down_btn );

        super.addSeparator( mini_separator_size );

        icon_URL = getURL( img_path + "Edit24.gif" );
        if ( icon_URL != null )
            mark_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            mark_btn = new JButton( "Mark" );
        mark_btn.setToolTipText( "Mark the timelines" );
        // mark_btn.setPreferredSize( btn_dim );
        mark_btn.addActionListener(
                 new ActionTimelineMark( this, y_tree ) );
        super.add( mark_btn );

        icon_URL = getURL( img_path + "Paste24.gif" );
        if ( icon_URL != null )
            move_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            move_btn = new JButton( "Move" );
        move_btn.setToolTipText( "Move the marked timelines" );
        // move_btn.setPreferredSize( btn_dim );
        move_btn.addActionListener(
                 new ActionTimelineMove( this, y_tree ) );
        super.add( move_btn );

        icon_URL = getURL( img_path + "Delete24.gif" );
        if ( icon_URL != null )
            delete_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            delete_btn = new JButton( "Delete" );
        delete_btn.setToolTipText( "Delete the marked timelines" );
        // delete_btn.setPreferredSize( btn_dim );
        delete_btn.addActionListener(
                   new ActionTimelineDelete( this, y_tree ) );
        super.add( delete_btn );

        /*
        icon_URL = getURL( img_path + "Remove24.gif" );
        if ( icon_URL != null )
            remove_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            remove_btn = new JButton( "Remove" );
        remove_btn.setToolTipText( "Remove the timeline from the display" );
        // remove_btn.setPreferredSize( btn_dim );
        remove_btn.addActionListener(
            new action_timeline_remove( y_tree, list_view ) );
        super.add( remove_btn );
        */

        super.addSeparator( mini_separator_size );

        icon_URL = getURL( img_path + "TreeExpand24.gif" );
        if ( icon_URL != null )
            expand_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            expand_btn = new JButton( "Expand" );
        expand_btn.setToolTipText( "Expand the tree by 1 level" );
        expand_btn.setMnemonic( KeyEvent.VK_E );
        // expand_btn.setPreferredSize( btn_dim );
        expand_btn.addActionListener(
                   new ActionYaxisTreeExpand( this, y_tree ) );
        super.add( expand_btn );

        icon_URL = getURL( img_path + "TreeCollapse24.gif" );
        if ( icon_URL != null )
            collapse_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            collapse_btn = new JButton( "Collapse" );
        collapse_btn.setToolTipText( "Collapse the tree by 1 level" );
        collapse_btn.setMnemonic( KeyEvent.VK_C );
        // collapse_btn.setPreferredSize( btn_dim );
        collapse_btn.addActionListener(
                     new ActionYaxisTreeCollapse( this, y_tree ) );
        super.add( collapse_btn );

        icon_URL = getURL( img_path + "TreeCommit24.gif" );
        if ( icon_URL != null )
            commit_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            commit_btn = new JButton( "Commit" );
        commit_btn.setToolTipText(
                   "Commit changes and Redraw the TimeLines Display" );
        commit_btn.setMnemonic( KeyEvent.VK_D );
        // collapse_btn.setPreferredSize( btn_dim );
        commit_btn.addActionListener(
                   new ActionYaxisTreeCommit( this, canvas_vport, y_maps ) );
        super.add( commit_btn );

        super.addSeparator();
        super.addSeparator();

        icon_URL = getURL( img_path + "Backward24.gif" );
        if ( icon_URL != null )
            backward_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            backward_btn = new JButton( "Backward" );
        backward_btn.setToolTipText( "scroll Backward half a screen" );
        backward_btn.setMnemonic( KeyEvent.VK_LEFT );
        // backward_btn.setPreferredSize( btn_dim );
        backward_btn.addActionListener(
                     new ActionVportBackward( time_scrollbar ) );
        super.add( backward_btn );

        icon_URL = getURL( img_path + "Forward24.gif" );
        if ( icon_URL != null )
            forward_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            forward_btn = new JButton( "Forward" );
        forward_btn.setToolTipText( "scroll Forward half a screen" );
        forward_btn.setMnemonic( KeyEvent.VK_RIGHT );
        // forward_btn.setPreferredSize( btn_dim );
        forward_btn.addActionListener(
                    new ActionVportForward( time_scrollbar ) );
        super.add( forward_btn );

        super.addSeparator( mini_separator_size );

        icon_URL = getURL( img_path + "Undo24.gif" );
        if ( icon_URL != null )
            zoomUndo_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            zoomUndo_btn = new JButton( "ZoomUndo" );
        zoomUndo_btn.setToolTipText( "Undo the previous zoom operation" );
        zoomUndo_btn.setMnemonic( KeyEvent.VK_U );
        // zoomUndo_btn.setPreferredSize( btn_dim );
        zoomUndo_btn.addActionListener(
                     new ActionZoomUndo( this, time_model ) );
        super.add( zoomUndo_btn );

        super.addSeparator( mini_separator_size );

        icon_URL = getURL( img_path + "ZoomOut24.gif" );
        if ( icon_URL != null )
            zoomOut_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            zoomOut_btn = new JButton( "ZoomOut" );
        zoomOut_btn.setToolTipText( "Zoom Out 1 level in time" );
        zoomOut_btn.setMnemonic( KeyEvent.VK_O );
        // zoomOut_btn.setPreferredSize( btn_dim );
        zoomOut_btn.addActionListener(
                    new ActionZoomOut( this, time_model ) );
        super.add( zoomOut_btn );

        icon_URL = getURL( img_path + "Home24.gif" );
        if ( icon_URL != null )
            zoomHome_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            zoomHome_btn = new JButton( "Home" );
        zoomHome_btn.setToolTipText(
                     "Reset zoom to the initial resolution in time" );
        zoomHome_btn.setMnemonic( KeyEvent.VK_H );
        // zoomHome_btn.setPreferredSize( btn_dim );
        zoomHome_btn.addActionListener(
                 new ActionZoomHome( this, time_model ) );
        super.add( zoomHome_btn );

        icon_URL = getURL( img_path + "ZoomIn24.gif" );
        if ( icon_URL != null )
            zoomIn_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            zoomIn_btn = new JButton( "ZoomIn" );
        zoomIn_btn.setToolTipText( "Zoom In 1 level in time" );
        zoomIn_btn.setMnemonic( KeyEvent.VK_I );
        // zoomIn_btn.setPreferredSize( btn_dim );
        zoomIn_btn.addActionListener(
                   new ActionZoomIn( this, time_model ) );
        super.add( zoomIn_btn );

        super.addSeparator( mini_separator_size );

        icon_URL = getURL( img_path + "Redo24.gif" );
        if ( icon_URL != null )
            zoomRedo_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            zoomRedo_btn = new JButton( "ZoomRedo" );
        zoomRedo_btn.setToolTipText( "Redo the previous zoom operation" );
        zoomRedo_btn.setMnemonic( KeyEvent.VK_R );
        // zoomRedo_btn.setPreferredSize( btn_dim );
        zoomRedo_btn.addActionListener(
                     new ActionZoomRedo( this, time_model ) );
        super.add( zoomRedo_btn );

        super.addSeparator();
        super.addSeparator();

        icon_URL = getURL( img_path + "FindBack24.gif" );
        if ( icon_URL != null )
            searchBack_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            searchBack_btn = new JButton( "SearchBackward" );
        searchBack_btn.setToolTipText( "Search backward in time" );
        searchBack_btn.setMnemonic( KeyEvent.VK_B );
        // searchBack_btn.setPreferredSize( btn_dim );
        searchBack_btn.addActionListener( 
                       new ActionSearchBackward( this, canvas_vport ) );
        super.add( searchBack_btn );

        icon_URL = getURL( img_path + "Find24.gif" );
        if ( icon_URL != null )
            searchInit_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            searchInit_btn = new JButton( "SearchInitialize" );
        searchInit_btn.setToolTipText( "Initialize search in time" );
        searchInit_btn.setMnemonic( KeyEvent.VK_I );
        // searchInit_btn.setPreferredSize( btn_dim );
        searchInit_btn.addActionListener(
                       new ActionSearchInit( this, canvas_vport ) );
        super.add( searchInit_btn );

        icon_URL = getURL( img_path + "FindFore24.gif" );
        if ( icon_URL != null )
            searchFore_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            searchFore_btn = new JButton( "SearchForward" );
        searchFore_btn.setToolTipText( "Search forward in time" );
        searchFore_btn.setMnemonic( KeyEvent.VK_F );
        // searchFore_btn.setPreferredSize( btn_dim );
        searchFore_btn.addActionListener(
                       new ActionSearchForward( this, canvas_vport ) );
        super.add( searchFore_btn );

        super.addSeparator();
        super.addSeparator();

        icon_URL = getURL( img_path + "Refresh24.gif" );
        if ( icon_URL != null )
            refresh_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            refresh_btn = new JButton( "Refresh" );
        refresh_btn.setToolTipText(
                    "Refresh the screen after Preference update" );
        refresh_btn.setMnemonic( KeyEvent.VK_S );
        // refresh_btn.setPreferredSize( btn_dim );
        refresh_btn.addActionListener(
                   new ActionPptyRefresh( y_tree, commit_btn ) );
        super.add( refresh_btn );

        icon_URL = getURL( img_path + "Print24.gif" );
        if ( icon_URL != null )
            print_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            print_btn = new JButton( "Print" );
        print_btn.setToolTipText( "Print" );
        // print_btn.setPreferredSize( btn_dim );
        print_btn.addActionListener( new ActionPptyPrint() );
        super.add( print_btn );

        icon_URL = getURL( img_path + "Stop24.gif" );
        if ( icon_URL != null )
            stop_btn = new JButton( new ImageIcon( icon_URL ) );
        else
            stop_btn = new JButton( "Exit" );
        stop_btn.setToolTipText( "Exit" );
        // stop_btn.setPreferredSize( btn_dim );
        stop_btn.addActionListener( new ActionPptyStop() );
        super.add( stop_btn );
    }

    private void initAllButtons()
    {
        up_btn.setEnabled( true );
        down_btn.setEnabled( true );

        mark_btn.setEnabled( true );
        move_btn.setEnabled( false );
        delete_btn.setEnabled( false );
        // remove_btn.setEnabled( true );

        this.resetYaxisTreeButtons();

        backward_btn.setEnabled( true );
        forward_btn.setEnabled( true );
        this.resetZoomButtons();

        searchBack_btn.setEnabled( true );
        searchInit_btn.setEnabled( true );
        searchFore_btn.setEnabled( true );

        refresh_btn.setEnabled( true );
        print_btn.setEnabled( true );
        stop_btn.setEnabled( true );
    }

    public void resetYaxisTreeButtons()
    {
        expand_btn.setEnabled( y_tree.isLevelExpandable() );
        collapse_btn.setEnabled( y_tree.isLevelCollapsable() );
        commit_btn.setEnabled( true );
    }

    public void resetZoomButtons()
    {
        int zoomlevel = time_model.getZoomLevel();
        zoomIn_btn.setEnabled( zoomlevel < Const.MAX_ZOOM_LEVEL );
        zoomHome_btn.setEnabled( zoomlevel != Const.MIN_ZOOM_LEVEL );
        zoomOut_btn.setEnabled( zoomlevel > Const.MIN_ZOOM_LEVEL );

        zoomUndo_btn.setEnabled( ! time_model.isZoomUndoStackEmpty() );
        zoomRedo_btn.setEnabled( ! time_model.isZoomRedoStackEmpty() );
    }
}
