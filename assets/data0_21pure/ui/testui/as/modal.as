void setModalY( float y )
{
	Element @body = @window.document.body;

	Element @abs = @body.getElementById( 'modal-block-abs' );
	Element @rel = @body.getElementById( 'modal-block-rel' );
	if( @abs == null || @rel == null ) {
		return;
	}
	abs.css( 'top', String( ( body.clientHeight() - abs.clientHeight() ) * y ) + 'px' );
	rel.css( 'top', '0px' );
}
