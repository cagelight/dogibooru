var cmd_window
var cmd_cur

function do_login(entry, username, password) {
  let obj = {}
  obj.username = username
  obj.password = password
  DOGI.JSONPost("/api/auth", obj, (res, stat)=>{
    entry.appendChild(document.createElement('br'))
    if (!stat) {
      entry.appendChild(DOGI.WrapSpan('API error', 'cmd_bad'))
      return
    }
    if (res.status == 1) {
      entry.appendChild(DOGI.WrapSpan('login successful', 'cmd_res'))
    } else {
      entry.appendChild(DOGI.WrapSpan('login failed: ' + res.info, 'cmd_bad'))
    }
  }, false)
}

function process_cmd(entry) {
  let cmdtxt = entry.textContent
  cmdsp = cmdtxt.split(' ')
  cmd = cmdsp[0] || ''
  args = cmdsp.slice(1)
  if (cmd === 'clear') {
    DOGI.ClearElement(cmd_window)
  } else if (cmd === 'login') {
    do_login(entry, args[0], args[1])
  } else {
    DOGI.JSONPost("/api/cmd", cmdtxt, (res, stat)=>{
      if (!stat) {
        entry.appendChild(DOGI.WrapSpan('API error', 'cmd_bad'))
        return
      }
      entry.appendChild(document.createElement('br'))
      entry.appendChild(DOGI.WrapSpan(res.info, res.status ? 'cmd_res' : 'cmd_bad'))
    }, false)
  }
}

function cmd_enter() {
  if (cmd_cur) {
    process_cmd(cmd_cur.entry)
    cmd_cur.entry.oninput = null
    cmd_cur.entry.contentEditable = false
    cmd_cur.entry.className = 'cmd_text'
  } else {
    cmd_cur = {}
  }

  let cmd_cont = document.createElement('div')
  let cmd_pre = document.createElement('span')
  let cmd_text = document.createElement('span')
  cmd_cont.className = 'cmd_cont'
  cmd_pre.appendChild(document.createTextNode('> '))
  cmd_pre.className = 'cmd_pre'
  cmd_text.contentEditable = true
  cmd_text.className = 'cmd_edit'
  cmd_text.appendChild(document.createTextNode(''))
  cmd_cont.appendChild(cmd_pre)
  cmd_cont.appendChild(cmd_text)

  cmd_cur.container = cmd_cont
  cmd_cur.pre = cmd_pre
  cmd_cur.entry = cmd_text

  cmd_cur.entry.addEventListener("input", (event) => {
    if (cmd_cur.entry.innerText.slice(-1) == '\n') {
      cmd_cur.entry.innerHTML = cmd_cur.entry.textContent
      cmd_enter()
    }
	})
  if (cmd_window.firstChild) {
    cmd_window.insertBefore(cmd_cur.container, cmd_window.firstChild)
  }
  else {
    cmd_window.appendChild(cmd_cur.container)
  }
  cmd_cur.entry.focus()
}

document.addEventListener("DOMContentLoaded", function(event) {
  cmd_window = document.getElementById('cmd_window')
  cmd_enter()
});
