
var pl_cur_id = 0;

function setclass( obj, value )
{
    obj.setAttribute( 'class', value ); /* Firefox */
    obj.setAttribute( 'className', value ); /* IE */
}

function clear_children( elt )
{
    if (elt != null)
	{
        while (elt.hasChildNodes())
            elt.removeChild(elt.firstChild);
	}
}

function loadXMLDoc( url, callback )
{
  // branch for native XMLHttpRequest object
  if ( window.XMLHttpRequest )
  {
    req = new XMLHttpRequest();
    req.onreadystatechange = callback;
    req.open( "GET", url, true );
    req.send( null );
  // branch for IE/Windows ActiveX version
  }
  else if ( window.ActiveXObject )
  {
    req = new ActiveXObject( "Microsoft.XMLHTTP" );
    if ( req )
    {
      req.onreadystatechange = callback;
      req.open( "GET", url, true );
      req.send();
    }
  }
}

function addslashes( str )
{
	return str.replace(/\'/g, '\\\'');
}

function escapebackslashes( str )
{
	return str.replace(/\\/g, '\\\\');
}

function parse_status()
{
    if( req.readyState == 4 )
    {
        if( req.status == 200 )
        {
		}
		else
		{
		}
	}
}

function parse_playlist()
{
    if (req.readyState == 4)
    {
        if (req.status == 200)
        {
            var answer = req.responseXML.documentElement;
            var playtree = document.getElementById('playtree');
            var pos = document.createElement("div");
            var pos_top = pos;
            var elt = answer.firstChild;

            pl_cur_id = 0;  /* changed to the current id if there actually is a current id */

            while (elt != null)
            {
                if (elt.nodeName == "node")
                {

                    if( pos.hasChildNodes() )
                        pos.appendChild( document.createElement( "br" ) );
                    var nda = document.createElement( 'a' );
                    nda.setAttribute( 'href', 'javascript:toggle_show_node(\''+elt.getAttribute( 'id' )+'\');' );
                    var ndai = document.createElement( 'img' );
                    ndai.setAttribute( 'src', 'images/minus.png' );
                    ndai.setAttribute( 'alt', '[-]' );
                    ndai.setAttribute( 'id', 'pl_img_'+elt.getAttribute( 'id' ) );
                    nda.appendChild( ndai );
                    pos.appendChild( nda );
                    pos.appendChild( document.createTextNode( ' ' + elt.getAttribute( 'name' ) ) );

                    if( elt.getAttribute( 'ro' ) == 'rw' )
                    {
                        pos.appendChild( document.createTextNode( ' ' ) );
                        var del = document.createElement( "a" );
                        del.setAttribute( 'href', 'javascript:pl_delete('+elt.getAttribute( 'id' )+')' );
                            var delimg = document.createElement( "img" );
                            delimg.setAttribute( 'src', 'images/delete_small.png' );
                            delimg.setAttribute( 'alt', '(delete)' );
                        del.appendChild( delimg );
                        pos.appendChild( del );
                    }

                    var nd = document.createElement( "div" );
                    setclass( nd, 'pl_node' );
                    nd.setAttribute( 'id', 'pl_'+elt.getAttribute( 'id' ) );
                    pos.appendChild( nd );

                }
                else if (elt.nodeName == "leaf")
                {

                    if( pos.hasChildNodes() )
                    pos.appendChild( document.createElement( "br" ) );
                    var pl = document.createElement( "a" );
                    setclass( pl, 'pl_leaf' );
                    pl.setAttribute( 'href', 'javascript:pl_play('+elt.getAttribute( 'id' )+');' );
                    pl.setAttribute( 'id', 'pl_'+elt.getAttribute( 'id' ) );

                    if( elt.getAttribute( 'current' ) == 'current' )
                    {
                        //pl.style.fontWeight = 'bold';
                        var nowplaying = document.getElementById( 'nowplaying' );
                        clear_children( nowplaying );
                        nowplaying.appendChild( document.createTextNode( elt.getAttribute( 'name' ) ) );
                        pl.appendChild( document.createTextNode( '* '));
                        pl_cur_id = elt.getAttribute( 'id' );
                    }

                    pl.setAttribute( 'title', elt.getAttribute( 'uri' ));
                    pl.appendChild( document.createTextNode( elt.getAttribute( 'name' ) ) );
                    var duration = elt.getAttribute( 'duration' );
                    if( duration > 0 )
                        pl.appendChild( document.createTextNode( " (" + format_time( elt.getAttribute( 'duration' ) / 1000000 ) + ")" ) );
                    pos.appendChild( pl );

                    if( elt.getAttribute( 'ro' ) == 'rw' )
                    {
                        pos.appendChild( document.createTextNode( ' ' ) );
                        var del = document.createElement( "a" );
                        del.setAttribute( 'href', 'javascript:pl_delete('+elt.getAttribute( 'id' )+')' );
                            var delimg = document.createElement( "img" );
                            delimg.setAttribute( 'src', 'images/delete_small.png' );
                            delimg.setAttribute( 'alt', '(delete)' );
                        del.appendChild( delimg );
                        pos.appendChild( del );
                    }

                }

                if (elt.firstChild != null)
                {
                    elt = elt.firstChild;
                    pos = pos.lastChild;
                }
                else if (elt.nextSibling != null)
                {
                    elt = elt.nextSibling;
                    pos = pos;
                }
                else
                {
                    while (elt.parentNode.nextSibling == null)
                    {
                        elt = elt.parentNode;
                        if (elt.parentNode == null)
							break;
                        pos = pos.parentNode;
                    }

                    if (elt.parentNode == null)
						break;

                    elt = elt.parentNode.nextSibling;
                    pos = pos.parentNode;
                }
            }

//            clear_children( playtree );
//            playtree.appendChild( pos_top );
        }
        else
        {
            /*alert( 'Error! HTTP server replied: ' + req.status );*/
        }
    }
}

function update_playlist()
{
/*
    loadXMLDoc( 'requests/playlist.xml', parse_playlist );
*/
}

function in_play(input)
{
    loadXMLDoc( '/cgi-bin/mplayer.cgi?stop', parse_status );
    loadXMLDoc( '/cgi-bin/mplayer.cgi?loadlist+'+input, parse_status );
/*
    loadXMLDoc( '/requests/status.xml?command=pl_stop', parse_status );
    loadXMLDoc( '/requests/status.xml?command=pl_empty', parse_status );

    var url = '/requests/status.xml?command=in_play&input='+encodeURIComponent( addslashes(escapebackslashes(input)) );
    loadXMLDoc( url, parse_status );

    setTimeout( 'update_playlist()', 1000 );
*/
}

function in_enqueue(input)
{
    var url = '/requests/status.xml?command=in_enqueue&input='+encodeURIComponent( addslashes(escapebackslashes(input)) );
    loadXMLDoc( url, parse_status );
    setTimeout( 'update_playlist()', 1000 );
}

function pl_empty()
{
    loadXMLDoc( '/requests/status.xml?command=pl_empty', parse_status );
    setTimeout( 'update_playlist()', 1000 );
}

function pl_pause()
{
    loadXMLDoc( '/requests/status.xml?command=pl_pause&id='+pl_cur_id, parse_status );
}

function pl_stop()
{
    loadXMLDoc( '/cgi-bin/mplayer.cgi?stop', parse_status );
/*
    loadXMLDoc( '/requests/status.xml?command=pl_stop', parse_status );
*/
    setTimeout( 'update_playlist()', 1000 );
}

function pl_next()
{
    loadXMLDoc( '/requests/status.xml?command=pl_next', parse_status );
    setTimeout( 'update_playlist()', 1000 );
}

function pl_previous()
{
    loadXMLDoc( '/requests/status.xml?command=pl_previous', parse_status );
    setTimeout( 'update_playlist()', 1000 );
}


/* *********************************************************** */


var monthNames;
var dayNames;

function UpdateClock()
{
	var theDate = new Date();

	clockDate.innerText = dayNames[theDate.getDay()] + ", " + monthNames[theDate.getMonth()] + " " + theDate.getDate();

	var hour = theDate.getHours();
	var minute = theDate.getMinutes();
	var ampm = hour >= 12 ? "pm" : "am";

	if (hour == 0)
		hour = 12;
	else if (hour > 12)
		hour -= 12;

	if (minute < 10)
		minute = "0" + minute;

	clockTime.innerText = hour + ":" + minute + " " + ampm;
}

function StartClock()
{
	monthNames = new Array();
	monthNames[0] = "January";
	monthNames[1] = "February";
	monthNames[2] = "March";
	monthNames[3] = "April";
	monthNames[4] = "May";
	monthNames[5] = "June";
	monthNames[6] = "July";
	monthNames[7] = "August";
	monthNames[8] = "September";
	monthNames[9] = "October";
	monthNames[10] = "November";
	monthNames[11] = "December";

	dayNames = new Array();
	dayNames[0] = "Sunday";
	dayNames[1] = "Monday";
	dayNames[2] = "Tuesday";
	dayNames[3] = "Wednesday";
	dayNames[4] = "Thursday";
	dayNames[5] = "Friday";
	dayNames[6] = "Saturday";

	UpdateClock();
	window.setInterval(UpdateClock, 500);
}
