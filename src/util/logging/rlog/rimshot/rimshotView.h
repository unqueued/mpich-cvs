// rimshotView.h : interface of the CRimshotView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_RIMSHOTVIEW_H__7E08A000_563B_4F32_ABF2_F9B1CA0B9486__INCLUDED_)
#define AFX_RIMSHOTVIEW_H__7E08A000_563B_4F32_ABF2_F9B1CA0B9486__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "rimshot_draw.h"

class CRimshotView : public CView
{
protected: // create from serialization only
	CRimshotView();
	DECLARE_DYNCREATE(CRimshotView)

// Attributes
public:
	CRimshotDoc* GetDocument();
	HANDLE m_hDrawThread;
	RimshotDrawStruct m_Draw;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRimshotView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	virtual void OnInitialUpdate();
	virtual BOOL DestroyWindow();
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	void StartDrawing();
	void StopDrawing();
	void DrawToCanvas(CDC *pDC);
	virtual ~CRimshotView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CRimshotView)
	afx_msg void OnNext();
	afx_msg void OnPrevious();
	afx_msg void OnResetZoom();
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnZoomTo();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnToggleArrows();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in rimshotView.cpp
inline CRimshotDoc* CRimshotView::GetDocument()
   { return (CRimshotDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RIMSHOTVIEW_H__7E08A000_563B_4F32_ABF2_F9B1CA0B9486__INCLUDED_)
