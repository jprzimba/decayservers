var Guild = {
    requestInvite: function(player_name, guild_id) {
        $('invite_button').style['visibility'] = 'hidden';
        new Ajax.Request('modules/guild_invite.php', {
            method: 'post',
            parameters: {
                player_name: player_name,
                guild_id: guild_id
            },
            onFailure: function() {alert('AJAX failed.')},
            onSuccess: this.callbackInvite,
            onComplete: function() {
                $('invite_button').style['visibility'] = 'visible';
            }
        });
    },
    callbackInvite : function(transport) {
        var param = transport.request.options.parameters;
        var XML = parseXML(transport.responseText);
        if (XML.getElementsByTagName('error')[0].childNodes[0].nodeValue == 0) {
            location.reload(false);
        } else {
            alert(XML.getElementsByTagName('message')[0].childNodes[0].nodeValue);
        }
    },
    requestAddRank: function(guild_id, rank_name) {
        $('rank_button').style['visibility'] = 'hidden';
        new Ajax.Request('modules/guild_rank.php', {
            method: 'post',
            parameters: {
                rank_name: rank_name,
                guild_id: guild_id
            },
            onFailure: function() {alert('AJAX failed.')},
            onSuccess: this.callbackAddRank,
            onComplete: function() {
                $('rank_button').style['visibility'] = 'visible';
            }
        });
    },
    callbackAddRank : function(transport) {
        var param = transport.request.options.parameters;
        var XML = parseXML(transport.responseText);
        if (XML.getElementsByTagName('error')[0].childNodes[0].nodeValue == 0) {
            location.reload(false);
        } else {
            alert(XML.getElementsByTagName('message')[0].childNodes[0].nodeValue);
        }
    },
    requestKick : function(node_id, player_name, guild_id) {
        if (confirm('Are you sure you want to remove ['+player_name+'] from the guild?')) {
            new Ajax.Request('modules/guild_kick.php', {
                method: 'post',
                parameters: {
                    node_id: node_id,
                    player_name: player_name,
                    guild_id: guild_id
                },
                onSuccess: this.callbackKick,
                onFailure: function() {alert('AJAX failed.')}
            });
        }
    },
    callbackKick : function(transport) {
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
    requestRankDelete : function(guild_id, rank_id) {
        if (confirm('Are you sure you want to remove this rank?')) {
            new Ajax.Request('modules/guild_rank_remove.php', {
                method: 'post',
                parameters: {
                    guild_id: guild_id,
                    rank_id: rank_id
                },
                onSuccess: this.callbackRankDelete,
                onFailure: function() {alert('AJAX failed.')}
            });
        }
    },
    callbackRankDelete : function(transport) {
        var param = transport.request.options.parameters;
        var XML = parseXML(transport.responseText);
        if (XML.getElementsByTagName('error')[0].childNodes[0].nodeValue == 0) {
            $('rank'+param.rank_id).parentNode.innerHTML = '';
        } else {
            alert(XML.getElementsByTagName('message')[0].childNodes[0].nodeValue);
        }
    },
    requestJoin : function(player_name, guild_id) {
        if (confirm('Are you sure you want to join this guild with ['+player_name+']?')) {
            new Ajax.Request('modules/guild_join.php', {
                method: 'post',
                parameters: {
                    player_name: player_name,
                    guild_id: guild_id
                },
                onSuccess: this.callbackJoin,
                onFailure: function() {alert('AJAX failed.')}
            });
        }
    },
    callbackJoin : function(transport) {
        var param = transport.request.options.parameters;
        var XML = parseXML(transport.responseText);
        if (XML.getElementsByTagName('error')[0].childNodes[0].nodeValue == 0) {
            location.reload(false);
        } else {
            alert(XML.getElementsByTagName('message')[0].childNodes[0].nodeValue);
        }
    },
    prepareRankRename : function(guild_id, rank_id, value) {
        $('rank'+rank_id).innerHTML = '<input id="rank'+rank_id+'_rename" type="text" value="'+ value +'" style="width: 100px;"/><img style="cursor: pointer" src="resource/accept.png" alt="OK" height="16" width="16" onclick="Guild.requestRankRename('+guild_id+', '+rank_id+', \''+value.replace(/'/g, '\\\'')+'\', $(\'rank'+rank_id+'_rename\').value)"/>'
    },
    requestRankRename : function(guild_id, rank_id, old_name, new_name) {
        $('rank'+rank_id).innerHTML = new_name;
        new Ajax.Request('modules/guild_rank.php', {
            method: 'post',
            parameters: {
                guild_id: guild_id,
                rank_id: rank_id,
                rank_name: new_name,
                old_name: old_name
            },
            onSuccess: this.callbackRankRename,
            onFailure: function() {
                alert('AJAX failed.')
                }
        });
    },
    callbackRankRename : function(transport) {
        var param = transport.request.options.parameters;
        var XML = parseXML(transport.responseText);
        var name;
        if (XML.getElementsByTagName('error')[0].childNodes[0].nodeValue == 0) {
            name = XML.getElementsByTagName('name')[0].childNodes[0].nodeValue;
        } else {
            alert(XML.getElementsByTagName('message')[0].childNodes[0].nodeValue);
            name = param.old_name;
        }
        $('rank'+param.rank_id).innerHTML =
        '<img style="cursor: pointer" src="resource/page_edit.png" alt="edit" height="16" width="16" onclick="Guild.prepareRankRename('+param.guild_id+', '+param.rank_id+',\''+name.replace(/'/g, '\\\'')+'\')"/>&nbsp;' +
        '<img style="cursor: pointer" src="resource/cross.png" alt="del" height="16" width="16" onclick="Guild.requestRankDelete('+param.guild_id+', '+param.rank_id+')"/>&nbsp;' +
        name;

    },
    prepareNickChange : function(guild_id, player_id, value) {
        $('player'+player_id+'_title').innerHTML = '<input id="player'+player_id+'_rename" type="text" value="'+ value +'" style="width: 100px;"/><img style="cursor: pointer" src="resource/accept.png" alt="OK" height="16" width="16" onclick="Guild.requestNickChange('+guild_id+', '+player_id+', \''+value.replace(/'/g, '\\\'')+'\', $(\'player'+player_id+'_rename\').value)"/>'
    },
    requestNickChange : function(guild_id, player_id, old_name, new_name) {
        $('player'+player_id+'_title').innerHTML = new_name;
        new Ajax.Request('modules/guild_set_nickname.php', {
            method: 'post',
            parameters: {
                guild_id: guild_id,
                player_id: player_id,
                nickname: new_name,
                old_name: old_name
            },
            onSuccess: this.callbackNickChange,
            onFailure: function() {
                alert('AJAX failed.')
                }
        });
    },
    callbackNickChange : function(transport) {
        var param = transport.request.options.parameters;
        var XML = parseXML(transport.responseText);
        var name;
        if (XML.getElementsByTagName('error')[0].childNodes[0].nodeValue == 0) {
            name = XML.getElementsByTagName('nickname')[0].childNodes[0].nodeValue;
        } else {
            alert(XML.getElementsByTagName('message')[0].childNodes[0].nodeValue);
            name = param.old_name;
        }
        $('player'+param.player_id+'_title').innerHTML = name + '&nbsp;' +
          '<img style="cursor: pointer" src="resource/page_edit.png" alt="edit" height="16" width="16" onclick="Guild.prepareNickChange('+param.guild_id+', '+param.player_id+',\''+name.replace(/'/g, '\\\'')+'\')"/>';
    }
}