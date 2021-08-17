// template Inline {
//     let i: uint32
// }

// template Object {
//     let f: uint32
//     let x: bool
//     let i: Inline
// }

// spec Create {
//     const create() => Object
// }

// action Object {
//     const doSomething(let self, let f: uint) => {
//         self.f = f
//     }
// }

// action Object: Create {
//     const create() => Object {
//         return Object {
//             f: 0,
//             x: true,
//             i: {
//                 i: 5
//             }
//         }
//     }
// }

// const main() => {
//     let o = Object.create()
//     o.doSomething(5)

//     let i = [5, 5, 4: 4]
//     i[3] = 7
// }

template Object<T, U> {
    let f: T
    let x: bool
}

type Other<T, U> = Object<T, U>

const main() => {
    let o = Other<bool, Object<uint32, bool>> {
        f: false,
        x: true
    }
    let o1 = Object<uint32, bool> {
        f: false,
        x: true
    }

}

