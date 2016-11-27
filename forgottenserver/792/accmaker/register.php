<?php 
include ("include.inc.php");
$ptitle= "Registration - $cfg[server_name]";
include ("header.inc.php");
?>
<div id="content">
<div class="top">Registration</div>
<div class="mid">
<table>
<tr><td width="40%" style="vertical-align: top"><label for="email"><b>Email address:</b></label></td><td width="60%"><input id="email" type="text" />&nbsp;<span id="email_state"></span><div><?php
if ($cfg['Email_Validate']) {
    echo 'This server requires email validation. A letter with your password will be sent to the address provided above.';
} else {
    echo 'Please enter a valid email address if we need to contact you.';
}
?></div></td></tr>
<tr><td width="40%" style="vertical-align: top"><label for="accname"><b>Desired Account Number:</b></label></td>
<td width="60%"><input id="accname" type="text" />&nbsp;<span id="accname_state"></span><div>
</div></td></tr>
<?php
if (!$cfg['Email_Validate']) {?>
    <tr><td style="vertical-align: top"><label for="password"><b>Choose a password:</b></label>
    </td><td><input id="password" type="password" />&nbsp;<span id="password_state"></span><div>
    Password consists of letters a-z, numbers 0-9, symbols(~!@#%&;,:\^$.|?*+()) and is at least 6 characters long.
    </div></td></tr>
    <tr><td style="vertical-align: top"><label for="confirm"><b>Re-enter password:</b></label>
    </td><td><input id="confirm" type="password" />&nbsp;<span id="confirm_state"></span><br/><br/></td></tr>
<?php } ?>
<tr><td style="vertical-align: top"><label for="rlname"><b>*Your name:</b></label>
</td><td><input id="rlname" type="text" /><br/><br/></td></tr>
<tr><td style="vertical-align: top"><label for="location"><b>*Your location:</b></label>
</td><td><input id="location" type="text" /><br/>* Optional fields<br/><br/></td></tr>
<?php
if($cfg['use_captcha']) {
    echo '<tr><td style="vertical-align: top"><b>Verification:</b></td><td><input id="captcha" type="text" style="text-transform: uppercase" />'.
    '<div>Type the characters you see in the picture below</div>'.
    '<img id="captcha_img" width="250px" height="40px" src="doimg.php?'.time().'" alt="Verification Image" /><br/><br/>'.
    '</td></tr>';
}
?>
<tr><td colspan="2"><div style="overflow-y: scroll; height: 200px;">
<?php
echo htmlspecialchars(@file_get_contents('documents/server_rules.txt'));
?>
</div>
<input id="rules_check" type="checkbox" onclick="onRulesCheck(this)"/>&nbsp;<label for="rules_check"><b>I agree with server rules</b></label>&nbsp;
<button id="submit_button" disabled="disabled" onclick="onSubmit()">Submit</button>
<span id="submit_load" style="color: red; font-weight: bold; text-decoration: blink;"></span>
<div id="submit_errors" style="color: red; font-weight: bold;"></div>
<div id="submit_success" style="color: green; font-weight: bold;"></div>
</td></tr>
</table>
<script type="text/javascript">
//<![CDATA[
function onRulesCheck(node) {
    if (node.checked) {
        node.disabled = true;
        $('submit_button').disabled = false;
    }
}

function onSubmit() {
    var params = new Array();
    params['email'] = $('email').value;
    params['accname'] = $('accname').value;

    params['rlname'] = $('rlname').value;
    params['location'] = $('location').value;
    params['captcha'] = $('captcha').value;
    params['submit'] = 'yes';
<?php if (!$cfg['Email_Validate']) {?>
    params['password'] = $('password').value;
    params['confirm'] = $('confirm').value;
<?php } else { ?>
    $('submit_load').innerHTML = 'Please wait...';
<?php } ?>
    $('submit_button').disabled = true;
    new Ajax.Request('modules/account_create.php', {
        method: 'post',
        parameters: params,
        onSuccess: function(transport) {
            var param = transport.request.options.parameters;
            var XML = parseXML(transport.responseText);
            var errors = XML.getElementsByTagName('error');
            var success = XML.getElementsByTagName('success');
            $('submit_errors').innerHTML = '';
            $('submit_success').innerHTML = '';
            $('submit_load').innerHTML = '';
            
            for (var i = 0; i < errors.length; i++) {
                $('submit_errors').innerHTML += errors[i].attributes.getNamedItem('id').value + ': ' + errors[i].childNodes[0].nodeValue + '<br/>';
            }
            if (success.length > 0) {
                $('submit_success').innerHTML = success[0].childNodes[0].nodeValue;
            } else {
                $('submit_button').disabled = false;
            }
            $('captcha_img').src = 'doimg.php?' + Date.parse(new Date().toString());

        },
        onFailure: function() {alert('AJAX failed.')}
    });
}

function updateState(id, XML) {
        if($(id).value == '') {
            $(id+'_state').innerHTML = '';
            return;
        }
        var errors = XML.getElementsByTagName('error');
        for (var i = 0; i < errors.length; i++) {
            if (errors[i].attributes.getNamedItem('id').value == id) {
                $(id+'_state').innerHTML = '<img src="resource/cross.png" alt="X" title="'+errors[i].childNodes[0].nodeValue+'"/>';
                return;
            }
        }
        $(id+'_state').innerHTML = '<img src="resource/tick.png" alt="V" />';
    }

var observerCallback = function(el, value) {
    var params = new Array();
    params['el_id'] = el.id;
    params['email'] = $('email').value;
    params['accname'] = $('accname').value;
<?php if (!$cfg['Email_Validate']) {?>
    params['password'] = $('password').value;
    params['confirm'] = $('confirm').value;
<?php } ?>
    new Ajax.Request('modules/account_create.php', {
            method: 'post',
            parameters: params,
            onSuccess: function(transport) {
                var param = transport.request.options.parameters;
                var XML = parseXML(transport.responseText);
                if (param.el_id == 'accname') {
                    updateState('accname', XML);
                    updateState('password', XML);
                } else if (param.el_id == 'password') {
                    updateState('password', XML);
                    updateState('confirm', XML);
                } else {
                    updateState(param.el_id, XML);
                }
            },
            onFailure: function() {alert('AJAX failed.')}
    });
}

new Form.Element.Observer('email', 2, observerCallback);
new Form.Element.Observer('accname', 2, observerCallback);
<?php if (!$cfg['Email_Validate']) {?>
new Form.Element.Observer('password', 2, observerCallback);
new Form.Element.Observer('confirm', 2, observerCallback);
<?php } ?>
//]]>
</script>
</div>
<div class="bot"></div>
</div>
<?php include ("footer.inc.php");?>