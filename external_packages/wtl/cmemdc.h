class CMemDC : public CDCHandle {
private:       
	CBitmap       m_bitmap;        // Offscreen bitmap
	CBitmapHandle m_oldBitmap; // bitmap originally found in CMemDC
	CDCHandle     m_pDC;           // Saves CDC passed in constructor
	CRect         m_rect;          // Rectangle of drawing area.
public:

	CMemDC(CDCHandle pDC, const CRect* pRect = NULL) : CDCHandle()
	{
		// Some initialization
		m_pDC = pDC;
		m_oldBitmap = NULL;

		// Get the rectangle to draw
		if (pRect == NULL) {
			pDC.GetClipBox(&m_rect);
		} else {
			m_rect = *pRect;
		}

		// Create a Memory DC
		CreateCompatibleDC(pDC);
		pDC.LPtoDP(&m_rect);

		m_bitmap.CreateCompatibleBitmap(pDC, m_rect.Width(), 
			m_rect.Height());
		m_oldBitmap = SelectBitmap(m_bitmap);

		SetMapMode(pDC.GetMapMode());

		{
			SIZE sz;
			pDC.GetWindowExt( & sz );
			SetWindowExt( sz.cx, sz.cy );
			pDC.GetViewportExt( & sz );
			SetViewportExt( sz.cx, sz.cy );
		}

		pDC.DPtoLP(&m_rect);
		SetWindowOrg(m_rect.left, m_rect.top);

		// Fill background 
		FillSolidRect(m_rect, pDC.GetBkColor());
	}

	~CMemDC()      
	{          
		// Copy the offscreen bitmap onto the screen.
		m_pDC.BitBlt(m_rect.left, m_rect.top, 
			m_rect.Width(),  m_rect.Height(),
			m_hDC, m_rect.left, m_rect.top, SRCCOPY);            

		//Swap back the original bitmap.
		SelectBitmap(m_oldBitmap);        
	}
};
