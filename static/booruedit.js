var img;
var tedit_iarea;
var tedit_sub_container;
var tedit_button_div;
var tedit_submit;
var tedit_container;
var numsubs = 0;

function setup_edit(get_obj) {
	remallsubs()
	if (get_obj[img].itags) tedit_iarea.value = get_obj[img].itags.join(' ')
	else tedit_iarea.value = ""
	if (get_obj[img].subs) get_obj[img].subs.forEach((sub)=>{
		let newsub = addsub()
		newsub.textarea.value = sub.join(' ')
	})
}

function setup_page() {
	remallsubs()
	img = window.location.hash.substring(1)
	document.title = 'Edit: #' + img
	DOGI.JSONPost("/api/tags/", {"get":[parseInt(img)]}, function(result, ok) {
		if (ok && img in result) {
			setup_edit(result)
		} else {
			tedit_iarea.value = ""
		}
	});
	DOGI.JSONPost("/api/img/", [parseInt(img)], function(result) {
		let image_view = document.getElementById("image_view")
		DOGI.ClearElement(image_view)
		let image = document.createElement("img")
		image.src = "/i/" + result[img].name
		image.id = "view_img"
		image.style.maxWidth = "100%"
		image.style.maxHeight = "100%"
		image.onclick = function(){
			if (image.style.maxWidth) {
				image.style.maxWidth = null
				image.style.maxHeight = null
			} else {
				image.style.maxWidth = "100%"
				image.style.maxHeight = "100%"
			}
		}

		image_view.appendChild(image)
	});
}

window.onhashchange = function(){setup_page()};
document.addEventListener("DOMContentLoaded", function(event) {
	tedit_iarea = document.getElementById('tedit_iarea')
	tedit_sub_container = document.getElementById('tedit_sub_container')
	tedit_button_div = document.getElementById('tedit_button_div')
	tedit_submit = document.getElementById('tedit_submit')
	tedit_container = document.getElementById('tedit_container')
	tedit_iarea.addEventListener("input", (event) => {
		tedit_iarea.value = tedit_iarea.value.replace('\n', ' ')
		tedit_iarea.value = tedit_iarea.value.replace('  ', ' ')
	})
	setup_page()
});

function addsub() {
	if (!numsubs) {
		let subtext = document.createElement('span')
		subtext.appendChild(document.createTextNode('Subject Tags:'))
		subtext.className = 'tedit_head'
		tedit_sub_container.appendChild(subtext)
		tedit_iarea.style.flex = "0 1 auto"
		tedit_sub_container.style.flex = "1 0 auto"
	}
	//================
	let new_subdiv = document.createElement('div')
	new_subdiv.className = 'tedit_sub_div'
	//================
	let new_subdivhead = document.createElement('div')
	new_subdivhead.className = 'tedit_sub_div_header'
	new_subdiv.appendChild(new_subdivhead)
	//================
	let subdelbutton = document.createElement('input')
	subdelbutton.className = 'tedit_sub_del_button'
	subdelbutton.type = 'button'
	subdelbutton.value = "X"
	subdelbutton.addEventListener("click", (event) => {
		tedit_sub_container.removeChild(new_subdiv)
		if (numsubs == 1) remallsubs()
		else numsubs --
	})
	new_subdivhead.appendChild(subdelbutton)
	//================
	let new_sarea = document.createElement('textarea')
	new_sarea.className = 'tedit_earea'
	new_sarea.addEventListener("input", (event) => {
		new_sarea.value = new_sarea.value.replace('\n', ' ')
		new_sarea.value = new_sarea.value.replace('  ', ' ')
	})
	new_subdiv.appendChild(new_sarea)
	//================
	tedit_sub_container.appendChild(new_subdiv)
	numsubs ++

	let rdata = {}
	rdata.textarea = new_sarea
	return rdata
}

function remallsubs() {
	if (numsubs) {
		numsubs = 0
		DOGI.ClearElement(tedit_sub_container)
		tedit_iarea.style.flex = null
		tedit_sub_container.style.flex = null
	}
}

function editsubmit() {
	tedit_iarea.value = tedit_iarea.value.trim()
	let tags = []
	tedit_iarea.value.split(' ').forEach((s)=>{if(s)tags.push(s)})
	//================
	let obj = {}
	obj["get"] = [parseInt(img)]
	obj["set"] = []
	obj["set"][0] = {}
	obj["set"][0].id = img
	obj["set"][0].itags = tags
	//================
	let subs = []
	DOGI.ChildIterate(tedit_sub_container, (child) => {
		if (child.className == 'tedit_sub_div') {
			let sub = []
			child.childNodes[1].value = child.childNodes[1].value.trim()
			child.childNodes[1].value.split(' ').forEach((s)=>{if(s)sub.push(s)})
			subs.push(sub)
		}
	})
	if (!subs.length) {
		DOGI.ErrorPopup("All images must have at least one subject")
		return
	}
	obj["set"][0].subs = subs
	//================
	DOGI.JSONPost("/api/tags/", obj, function(result, ok) {
		if (ok && img in result) {
			setup_edit(result)
			tedit_submit.style.animation = '0.25s ease-in-out 0s infinite normal tedit_submit_succ'
			setTimeout(()=>{tedit_submit.style.animation = null}, 250)
		} else {
			tedit_submit.style.animation = '0.125s ease-in-out 0s infinite normal tedit_submit_fail'
			setTimeout(()=>{tedit_submit.style.animation = null}, 500)
		}
	});
}
