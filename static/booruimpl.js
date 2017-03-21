var impl_tagcont
var impl_taginp
var impl_impinp

function set_column_num(num) {
  let colel = document.getElementById('impl_tagcont')
  colel.style.columnCount = num
}

function add_tag(tag, wrap_ul = false) {
  var tagcont
  if (wrap_ul) {
    tagcont = document.createElement('ul')
    tagcont.className = 'impl_list'
    let hue = tag.name.hashCode() % 360
    tagcont.style.borderColor = 'hsl(' + hue + ',100%,50%)'
  }
  let tagtext = document.createElement('li')
  tagtext.className = 'impl_listitem'
  tagtext.appendChild(document.createTextNode(tag.name))
  if (tagcont) {
    tagcont.appendChild(tagtext)
    impl_tagcont.appendChild(tagcont)
  } else {
    impl_tagcont.appendChild(tagtext)
  }

  if (tag.impl) {
    let subtags = []
    for (let key in tag.impl) {
      let subtag = {}
      subtag.name = key
      subtag.impl = tag.impl[key]
      subtags.push(subtag)
    }
    subtags.sort((a,b)=>{return a.name.localeCompare(b.name)})

    subtags.forEach((subtag)=>{
      let tagtextsub = document.createElement('ul')
      tagtextsub.className = 'impl_list'
      tagtext.appendChild(tagtextsub)
      tagtextsub.appendChild(add_tag(subtag))
    })
  }
  return tagtext
}

function setup_tag_area(data) {
  DOGI.ClearElement(impl_tagcont)

  num = window.location.hash.substring(1)
  if (!num) num = 6
  set_column_num(num)
  window.location.hash = num

  tags = []
  for (let key in data) {
    tag = {}
    tag.name = key
    tag.impl = data[key]
    tags.push(tag)
  }
  tags.sort((a,b)=>{return a.name.localeCompare(b.name)})
  tags.forEach((tag)=>{
    add_tag(tag, true)
  })
}

function setup_page() {
  DOGI.JSONPost("/api/impls", {"get":[]}, (data)=>{
    setup_tag_area(data)
  })
}
window.onhashchange = function(){setup_page()}
document.addEventListener("DOMContentLoaded", function(event) {
  impl_tagcont = document.getElementById('impl_tagcont')
  impl_taginp = document.getElementById('impl_taginp')
  impl_impinp = document.getElementById('impl_impinp')
	setup_page()
})

function implsubmit() {
  let tag = impl_taginp.value
  let imp = impl_impinp.value
  impl_taginp.value = ""
  impl_impinp.value = ""
  if (!tag || !imp) return
  let obj = {'get':[]}
  obj['set'] = []

  let tagary = []
  tag.split(' ').forEach((s)=>{if(s)tagary.push(s)})
  tagary.forEach((s)=> {
    obj['set'].push({"tag":s,"imp":imp})
  })

  DOGI.JSONPost('/api/impls', obj, (data)=>{
    setup_tag_area(data)
  })
}

function impldelete() {
  let tag = impl_taginp.value
  let imp = impl_impinp.value
  impl_taginp.value = ""
  impl_impinp.value = ""
  if (!tag || !imp) return
  let obj = {'get':[]}
  obj['del'] = [{"tag":tag,"imp":imp}]
  DOGI.JSONPost('/api/impls', obj, (data)=>{
    setup_tag_area(data)
  })
}
