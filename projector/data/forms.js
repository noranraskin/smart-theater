
const submitHandler = (event, form) => {
    event.preventDefault();
    const formData = new FormData(form);
    formData.append(event.submitter.name, event.submitter.value);
    const inputs = form.querySelectorAll("input[type=submit]");
    
    for (const input of inputs) {
        input.disabled = true;
    }
    
    fetch(form.action, {
        method: "post",
        body: new URLSearchParams(formData),
    })
    .then((response) => {
        if (response.status === 200) {
            event.target.classList.add("success");
            setTimeout(() => {
                event.target.classList.remove("success");
            }, 300);
        }
        
        for (const input of inputs) {
            input.disabled = false;
        }
    })
    .catch((error) => {
        console.error(error);
        
        for (const input of inputs) {
            input.disabled = false;
        }
    });
};

const forms = document.forms
for (const f of forms) {
    f.addEventListener("submit", (event) => {
        submitHandler(event, f);
    })
}