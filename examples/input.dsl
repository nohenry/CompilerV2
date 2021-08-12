template Inline {
    let i: uint32
}

template Object {
    let f: uint32
    let x: bool
    let i: Inline
}

action Object {
    const doSomething() => {

    }
}

const f() => {


    let o = Object {
        f: 0,
        x: true,
        i: {
            i: 5
        }
    }

    let x = o.i.i

    // let var = o.f.x
}
