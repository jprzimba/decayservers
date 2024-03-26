var Cookies = {
    init: function () {
        var allCookies = document.cookie.split('; ');
        for (var i = 0; i < allCookies.length; i++) {
            var cookiePair = allCookies[i].split('=');
            this[cookiePair[0]] = cookiePair[1];
        }
    },
    get: function (cookie_name) {
        var results = document.cookie.match(cookie_name + '=(.*?)(;|$)');

        if (results)
            return (unescape(results[1]));
        else
            return null;
    },
    create: function (name, value, days) {
        if (days) {
            var date = new Date();
            date.setTime(date.getTime() + (days * 24 * 60 * 60 * 1000));
            var expires = "; expires=" + date.toGMTString();
        }
        else var expires = "";
        document.cookie = name + "=" + value + expires + "; path=/";
        this[name] = value;
    },
    erase: function (name) {
        this.create(name, '', -1);
        this[name] = undefined;
    }
}
Cookies.init();

//this loads html content between element_id tags. warn displays loading message.
function ajax(element_id, script_url, get, warn) {
    if (warn)
        document.getElementById(element_id).innerHTML = '<div style="position: absolute; background-color: #990000; color: white;">Loading...</div>';

    new Ajax.Updater(element_id, script_url, {
        method: 'post',
        parameters: '?' + get + '&ajax=true',
        onComplete: function () {
            document.getElementById('iobox').style.left = Cookies.get('iobox_x');
            document.getElementById('iobox').style.top = Cookies.get('iobox_y');
            document.getElementById('iobox').style['visibility'] = 'visible';
        }
    });

    //reset logout timer
    ticker = 0;
}

function getRef(obj) {
    return (typeof obj == "string") ?
        document.getElementById(obj) : obj;
}

function setStyle(obj, style, value) {
    $(obj).style[style] = value;
}

// found on some website, no idea how it works :D :D
function startDrag(e) {
    // determine event object
    if (!e) {
        var e = window.event
    };
    // determine target element
    var targ = e.target ? e.target : e.srcElement;
    if (targ.className != 'draggable') {
        return
    };
    // calculate event X,Y coordinates
    offsetX = e.clientX;
    offsetY = e.clientY;
    // assign default values for top and left properties
    if (!targ.style.left) {
        targ.style.left = offsetX + 'px'
    };
    if (!targ.style.top) {
        targ.style.top = offsetY + 'px'
    };
    // calculate integer values for top and left properties
    coordX = parseInt(targ.style.left);
    coordY = parseInt(targ.style.top);
    drag_node = targ;
    // move element
    document.onmousemove = dragDiv;
    document.onmouseup = stopDrag;
}
// continue dragging
function dragDiv(e) {
    if (!e) {
        var e = window.event
    };
    if (!drag_node) {
        return
    };
    // move element
    drag_node.style.left = coordX + e.clientX - offsetX + 'px';
    drag_node.style.top = coordY + e.clientY - offsetY + 'px';
    return false;
}
// stop dragging
function stopDrag() {
    drag_node = null;
}
document.onmousedown = startDrag;

function iobox_mouseup() {
    if (document.getElementById('iobox').parentNode.id == 'form') {
        Cookies.create('iobox_x', document.getElementById('iobox').style.left, 1);
        Cookies.create('iobox_y', document.getElementById('iobox').style.top, 1);
    }
}

function calcFlags() {
    var flags = 0;
    var flagNode = document.getElementById('groups');
    for (var i = 0; i < flagNode.elements.length; i++) {
        if (flagNode.elements[i].checked) {
            flags = flags * 1 + flagNode.elements[i].value * 1;
        }
    }
    document.getElementById('groups__flags').value = flags;
}

function menu_toggle(node) {
    if (node.nextSibling.style['display'] == 'none') {
        node.nextSibling.style['display'] = 'block'
    } else {
        node.nextSibling.style['display'] = 'none'
    }

}

function input_clear(node) {
    if (node.style.fontStyle == 'italic') {
        node.value = '';
        node.style.fontStyle = 'normal';
    }
}

function parseXML(txt) {
    try //Internet Suxplorer
    {
        xmlDoc = new ActiveXObject("Microsoft.XMLDOM");
        xmlDoc.async = "false";
        xmlDoc.loadXML(txt);
    }
    catch (e) {
        parser = new DOMParser();
        xmlDoc = parser.parseFromString(txt, "text/xml");
    }
    if (xmlDoc.getElementsByTagName('response').length != 1) {
        document.write('Cannot parse XML string: ' + txt);
        return false;
    } else {
        return xmlDoc;
    }
}

var Guild = {
    requestInvite: function (player_name, guild_id) {
        $('invite_button').style['visibility'] = 'hidden';
        new Ajax.Request('modules/guild_invite.php', {
            method: 'post',
            parameters: {
                player_name: player_name,
                guild_id: guild_id
            },
            onFailure: function () { alert('AJAX failed.') },
            onSuccess: this.callbackInvite,
            onComplete: function () {
                $('invite_button').style['visibility'] = 'visible';
            }
        });
    },
    callbackInvite: function (transport) {
        var param = transport.request.options.parameters;
        var XML = parseXML(transport.responseText);
        if (XML.getElementsByTagName('error')[0].childNodes[0].nodeValue == 0) {
            location.reload(false);
        } else {
            alert(XML.getElementsByTagName('message')[0].childNodes[0].nodeValue);
        }
    },
    requestAddRank: function (guild_id, rank_name) {
        $('rank_button').style['visibility'] = 'hidden';
        new Ajax.Request('modules/guild_rank.php', {
            method: 'post',
            parameters: {
                rank_name: rank_name,
                guild_id: guild_id
            },
            onFailure: function () { alert('AJAX failed.') },
            onSuccess: this.callbackAddRank,
            onComplete: function () {
                $('rank_button').style['visibility'] = 'visible';
            }
        });
    },
    callbackAddRank: function (transport) {
        var param = transport.request.options.parameters;
        var XML = parseXML(transport.responseText);
        if (XML.getElementsByTagName('error')[0].childNodes[0].nodeValue == 0) {
            location.reload(false);
        } else {
            alert(XML.getElementsByTagName('message')[0].childNodes[0].nodeValue);
        }
    },
    requestKick: function (node_id, player_name, guild_id) {
        if (confirm('Are you sure you want to remove ' + player_name + ' from the guild?')) {
            new Ajax.Request('modules/guild_kick.php', {
                method: 'post',
                parameters: {
                    node_id: node_id,
                    player_name: player_name,
                    guild_id: guild_id
                },
                onSuccess: this.callbackKick,
                onFailure: function () { alert('AJAX failed.') }
            });
        }
    },
    callbackKick: function (transport) {
        var param = transport.request.options.parameters;
        var XML = parseXML(transport.responseText);
        if (XML.getElementsByTagName('error')[0].childNodes[0].nodeValue == 0) {
            var player_name = XML.getElementsByTagName('player')[0].childNodes[0].nodeValue;
            var node = document.getElementById(param.node_id);
            node.parentNode.removeChild(node);
        } else {
            alert(XML.getElementsByTagName('message')[0].childNodes[0].nodeValue);
        }
    },
    requestRankDelete: function (guild_id, rank_id) {
        if (confirm('Are you sure you want to remove this rank?')) {
            new Ajax.Request('modules/guild_rank_remove.php', {
                method: 'post',
                parameters: {
                    guild_id: guild_id,
                    rank_id: rank_id
                },
                onSuccess: this.callbackRankDelete,
                onFailure: function () { alert('AJAX failed.') }
            });
        }
    },
    callbackRankDelete: function (transport) {
        var param = transport.request.options.parameters;
        var XML = parseXML(transport.responseText);
        if (XML.getElementsByTagName('error')[0].childNodes[0].nodeValue == 0) {
            $('rank' + param.rank_id).parentNode.innerHTML = '';
        } else {
            alert(XML.getElementsByTagName('message')[0].childNodes[0].nodeValue);
        }
    },
    requestJoin: function (player_name, guild_id) {
        if (confirm('Are you sure you want to join this guild with [' + player_name + ']?')) {
            new Ajax.Request('modules/guild_join.php', {
                method: 'post',
                parameters: {
                    player_name: player_name,
                    guild_id: guild_id
                },
                onSuccess: this.callbackJoin,
                onFailure: function () { alert('AJAX failed.') }
            });
        }
    },
    callbackJoin: function (transport) {
        var param = transport.request.options.parameters;
        var XML = parseXML(transport.responseText);
        if (XML.getElementsByTagName('error')[0].childNodes[0].nodeValue == 0) {
            location.reload(false);
        } else {
            alert(XML.getElementsByTagName('message')[0].childNodes[0].nodeValue);
        }
    },
    prepareRankRename: function (guild_id, rank_id, value) {
        $('rank' + rank_id).innerHTML = '<input id="rank' + rank_id + '_rename" type="text" value="' + value + '" style="width: 100px;"/><img style="cursor: pointer" src="resource/accept.png" alt="OK" height="16" width="16" onclick="Guild.requestRankRename(' + guild_id + ', ' + rank_id + ', \'' + value + '\', $(\'rank' + rank_id + '_rename\').value)"/>'
    },
    requestRankRename: function (guild_id, rank_id, old_name, new_name) {
        $('rank' + rank_id).innerHTML = new_name;
        new Ajax.Request('modules/guild_rank.php', {
            method: 'post',
            parameters: {
                guild_id: guild_id,
                rank_id: rank_id,
                rank_name: new_name,
                old_name: old_name
            },
            onSuccess: this.callbackRankRename,
            onFailure: function () {
                alert('AJAX failed.')
            }
        });
    },
    callbackRankRename: function (transport) {
        var param = transport.request.options.parameters;
        var XML = parseXML(transport.responseText);
        var name;
        if (XML.getElementsByTagName('error')[0].childNodes[0].nodeValue == 0) {
            name = XML.getElementsByTagName('name')[0].childNodes[0].nodeValue;
        } else {
            alert(XML.getElementsByTagName('message')[0].childNodes[0].nodeValue);
            name = param.old_name;
        }
        $('rank' + param.rank_id).innerHTML =
            '<img style="cursor: pointer" src="resource/page_edit.png" alt="edit" height="16" width="16" onclick="Guild.prepareRankRename(' + param.guild_id + ', ' + param.rank_id + ',\'' + name + '\')"/>&nbsp;' +
            '<img style="cursor: pointer" src="resource/cross.png" alt="del" height="16" width="16" onclick="Guild.requestRankDelete(' + param.guild_id + ', ' + param.rank_id + ')"/>&nbsp;' +
            name;

    },
    prepareNickChange: function (guild_id, player_id, value) {
        $('player' + player_id + '_title').innerHTML = '<input id="player' + player_id + '_rename" type="text" value="' + value + '" style="width: 100px;"/><img style="cursor: pointer" src="resource/accept.png" alt="OK" height="16" width="16" onclick="Guild.requestNickChange(' + guild_id + ', ' + player_id + ', \'' + value + '\', $(\'player' + player_id + '_rename\').value)"/>'
    },
    requestNickChange: function (guild_id, player_id, old_name, new_name) {
        $('player' + player_id + '_title').innerHTML = new_name;
        new Ajax.Request('modules/guild_set_nickname.php', {
            method: 'post',
            parameters: {
                guild_id: guild_id,
                player_id: player_id,
                nickname: new_name,
                old_name: old_name
            },
            onSuccess: this.callbackNickChange,
            onFailure: function () {
                alert('AJAX failed.')
            }
        });
    },
    callbackNickChange: function (transport) {
        var param = transport.request.options.parameters;
        var XML = parseXML(transport.responseText);
        var name;
        if (XML.getElementsByTagName('error')[0].childNodes[0].nodeValue == 0) {
            name = XML.getElementsByTagName('nickname')[0].childNodes[0].nodeValue;
        } else {
            alert(XML.getElementsByTagName('message')[0].childNodes[0].nodeValue);
            name = param.old_name;
        }
        $('player' + param.player_id + '_title').innerHTML = name + '&nbsp;' +
            '<img style="cursor: pointer" src="resource/page_edit.png" alt="edit" height="16" width="16" onclick="Guild.prepareNickChange(' + param.guild_id + ', ' + param.player_id + ',\'' + name + '\')"/>';
    }
}